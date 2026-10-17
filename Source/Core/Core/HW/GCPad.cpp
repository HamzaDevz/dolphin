// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/HW/GCPad.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "Common/Common.h"
#include "Common/HookableEvent.h"
#include "Common/Logging/Log.h"
#include "Core/Config/MainSettings.h"
#include "Core/Core.h"
#include "Core/HW/GCPadEmu.h"
#include "Core/HW/SI/SI_Device.h"
#include "Core/System.h"
#include "InputCommon/ControlReference/ControlReference.h"
#include "InputCommon/ControllerEmu/Control/Control.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/GCPadStatus.h"
#include "InputCommon/GamepadAutoAssign.h"
#include "InputCommon/InputConfig.h"
#include "VideoCommon/OnScreenDisplay.h"

namespace Pad
{
static InputConfig s_config("GCPadNew", _trans("Pad"), "GCPad", "Pad");
InputConfig* GetConfig()
{
  return &s_config;
}

#ifdef ANDROID
static Common::EventHook s_gamepad_hotplug_hook;
static std::mutex s_gamepads_mutex;
static std::vector<std::string> s_known_gamepads;

// The connected gamepads (Android gives them a controller number), in player order.
static std::vector<std::shared_ptr<ciface::Core::Device>> GetGamepads()
{
  std::vector<std::shared_ptr<ciface::Core::Device>> gamepads;
  for (const std::string& name : g_controller_interface.GetAllDeviceStrings())
  {
    ciface::Core::DeviceQualifier qualifier;
    qualifier.FromString(name);
    auto device = g_controller_interface.FindDevice(qualifier);
    if (device && device->GetPreferredId())
      gamepads.push_back(std::move(device));
  }
  std::ranges::stable_sort(gamepads, {}, [](const auto& device) { return device->GetId(); });
  return gamepads;
}

static std::vector<std::string> GetNames(const std::vector<std::shared_ptr<ciface::Core::Device>>& devices)
{
  std::vector<std::string> names;
  for (const auto& device : devices)
  {
    ciface::Core::DeviceQualifier qualifier;
    qualifier.FromDevice(device.get());
    names.push_back(qualifier.ToString());
  }
  return names;
}

static bool HasInputMapped(const ControllerEmu::EmulatedController& controller)
{
  return std::ranges::any_of(controller.groups, [](const auto& group) {
    return std::ranges::any_of(group->controls, [](const auto& control) {
      return control->control_ref->IsInput() && !control->control_ref->GetExpression().empty();
    });
  });
}

static void AssignGamepads(const std::vector<std::string>* new_gamepads)
{
  const auto gamepads = GetGamepads();
  const auto gamepad_names = GetNames(gamepads);
  const auto connected = g_controller_interface.GetAllDeviceStrings();

  const auto lock = ControllerEmu::EmulatedController::GetStateLock();
  std::vector<InputCommon::GamepadPort> ports;
  for (int i = 0; i < 4; ++i)
  {
    const auto* pad = s_config.GetController(i);
    const SerialInterface::SIDevices type = Config::Get(Config::GetInfoForSIDevice(i));
    ports.push_back({type == SerialInterface::SIDEVICE_GC_CONTROLLER, HasInputMapped(*pad),
                     pad->GetDefaultDevice().ToString(), type == SerialInterface::SIDEVICE_NONE});
  }

  for (const auto& assignment :
       InputCommon::PlanGamepadAssignment(ports, connected, gamepad_names, new_gamepads))
  {
    const auto it = std::ranges::find(gamepad_names, assignment.device);
    const auto& device = gamepads[it - gamepad_names.begin()];
    auto* pad = static_cast<GCPad*>(s_config.GetController(assignment.port));
    if (assignment.keep_mapping)
      pad->SetDefaultDevice(assignment.device);
    else
      pad->LoadGamepadDefaults(g_controller_interface, *device);
    pad->UpdateReferences(g_controller_interface);

    // Plug in a controller for this game only, like on the console.
    if (!ports[assignment.port].active)
    {
      Config::SetCurrent(Config::GetInfoForSIDevice(assignment.port),
                         SerialInterface::SIDEVICE_GC_CONTROLLER);
    }

    INFO_LOG_FMT(SERIALINTERFACE, "Gamepad {} is player {}", assignment.device,
                 assignment.port + 1);
    OSD::AddMessage(fmt::format("Player {}: {}", assignment.port + 1, device->GetName()),
                    OSD::Duration::NORMAL);
  }
}

void AutoAssignGamepads()
{
  std::lock_guard lk(s_gamepads_mutex);
  s_known_gamepads = GetNames(GetGamepads());
  AssignGamepads(nullptr);
}

// A gamepad connected during a game takes the next free port.
static void OnDevicesChanged()
{
  std::lock_guard lk(s_gamepads_mutex);
  const auto gamepads = GetNames(GetGamepads());
  std::vector<std::string> added;
  for (const std::string& gamepad : gamepads)
  {
    if (std::ranges::find(s_known_gamepads, gamepad) == s_known_gamepads.end())
      added.push_back(gamepad);
  }
  s_known_gamepads = gamepads;

  if (!added.empty() && Core::IsRunningOrStarting(Core::System::GetInstance()))
    AssignGamepads(&added);
}
#else
void AutoAssignGamepads()
{
}
#endif

void Shutdown()
{
#ifdef ANDROID
  s_gamepad_hotplug_hook.reset();
#endif
  s_config.UnregisterHotplugCallback();

  s_config.ClearControllers();
}

void Initialize()
{
  if (s_config.ControllersNeedToBeCreated())
  {
    for (unsigned int i = 0; i < 4; ++i)
      s_config.CreateController<GCPad>(i);
  }

  s_config.RegisterHotplugCallback();
#ifdef ANDROID
  s_gamepad_hotplug_hook = g_controller_interface.RegisterDevicesChangedCallback(OnDevicesChanged);
#endif

  // Load the saved controller config
  s_config.LoadConfig();
}

void LoadConfig()
{
  s_config.LoadConfig();
}

void GenerateDynamicInputTextures()
{
  s_config.GenerateControllerTextures();
}

bool IsInitialized()
{
  return !s_config.ControllersNeedToBeCreated();
}

GCPadStatus GetStatus(int pad_num)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetInput();
}

ControllerEmu::ControlGroup* GetGroup(int pad_num, PadGroup group)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetGroup(group);
}

void Rumble(const int pad_num, const ControlState strength)
{
  static_cast<GCPad*>(s_config.GetController(pad_num))->SetOutput(strength);
}

void ResetRumble(const int pad_num)
{
  static_cast<GCPad*>(s_config.GetController(pad_num))->SetOutput(0.0);
}

bool GetMicButton(const int pad_num)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetMicButton();
}
}  // namespace Pad

// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/HW/GCPad.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "Common/Common.h"
#include "Common/HookableEvent.h"
#include "Common/Logging/Log.h"
#include "Core/Config/MainSettings.h"
#include "Core/Config/WiimoteSettings.h"
#include "Core/HW/GCPadEmu.h"
#include "Core/HW/SI/SI_Device.h"
#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"
#include "Core/System.h"
#include "InputCommon/ControlReference/ControlReference.h"
#include "InputCommon/ControllerEmu/Control/Control.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#ifdef ANDROID
#include "InputCommon/ControllerInterface/Android/Android.h"
#endif
#include "InputCommon/GCAdapter.h"
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
static constexpr int PORT_COUNT = 4;
static constexpr u16 ADAPTER_BUTTONS =
    PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT | PAD_BUTTON_DOWN | PAD_BUTTON_UP | PAD_TRIGGER_Z |
    PAD_TRIGGER_R | PAD_TRIGGER_L | PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_X | PAD_BUTTON_Y |
    PAD_BUTTON_START;

static Common::EventHook s_gamepad_hotplug_hook;
static std::mutex s_players_mutex;
static InputCommon::GamepadPlayers s_players(PORT_COUNT);
// Locked on its own: the devices change while the players are applied, when a Wii Remote's source
// changes.
static std::mutex s_hotplug_mutex;
static InputCommon::GamepadHotplug s_hotplug;
// Players join only while a game runs.
static bool s_players_enabled = false;
// The ports whose player changed, applied from the pad polling.
static u8 s_changed_ports = 0;
// The ports of the GameCube adapter with a controller plugged in, and with a button down.
static u8 s_adapter_connected = 0;
static u8 s_adapter_pressed = 0;
static std::atomic<bool> s_undocked = false;
// The port of the GameCube adapter each player plays on, or -1.
static std::array<std::atomic<int>, PORT_COUNT> s_adapter_channel = {-1, -1, -1, -1};
static std::array<std::atomic<bool>, PORT_COUNT> s_adapter_send_origin = {};

static std::string AdapterGamepad(int chan)
{
  return fmt::format("GCAdapter/{}/GameCube Controller", chan);
}

static std::optional<int> GetAdapterChannel(std::string_view gamepad)
{
  for (int chan = 0; chan < PORT_COUNT; ++chan)
  {
    if (gamepad == AdapterGamepad(chan))
      return chan;
  }
  return std::nullopt;
}

// The ports that can take a player: those with a GameCube controller or with nothing plugged in.
static std::vector<bool> GetPorts()
{
  std::vector<bool> ports;
  for (int i = 0; i < PORT_COUNT; ++i)
  {
    const SerialInterface::SIDevices type = Config::Get(Config::GetInfoForSIDevice(i));
    ports.push_back(type == SerialInterface::SIDEVICE_GC_CONTROLLER ||
                    type == SerialInterface::SIDEVICE_NONE);
  }
  return ports;
}

// The connected gamepads (Android gives them a controller number) and adapter controllers.
static std::vector<std::string> GetConnectedGamepads()
{
  std::vector<std::string> gamepads;
  for (const std::string& name : g_controller_interface.GetAllDeviceStrings())
  {
    ciface::Core::DeviceQualifier qualifier;
    qualifier.FromString(name);
    const auto device = g_controller_interface.FindDevice(qualifier);
    if (device && device->GetPreferredId())
      gamepads.push_back(name);
  }
  for (int chan = 0; chan < PORT_COUNT; ++chan)
  {
    if (s_adapter_connected & (1 << chan))
      gamepads.push_back(AdapterGamepad(chan));
  }
  return gamepads;
}

static bool HasInputMapped(const ControllerEmu::EmulatedController& controller)
{
  return std::ranges::any_of(controller.groups, [](const auto& group) {
    return std::ranges::any_of(group->controls, [](const auto& control) {
      return control->control_ref->IsInput() && !control->control_ref->GetExpression().empty();
    });
  });
}

// A Wii game set to play without the Wii Remote of this number, with a GameCube controller instead.
static bool GameTakesNoWiimote(int port)
{
  const auto& source = Config::GetInfoForWiimoteSource(port);
  for (const Config::LayerType type : {Config::LayerType::LocalGame, Config::LayerType::GlobalGame})
  {
    const auto layer = Config::GetLayer(type);
    if (layer && layer->Exists(source.GetLocation()))
      return layer->Get(source) == WiimoteSource::None;
  }
  return false;
}

// Gives the Wii Remote of the same number, with a Nunchuk, to the player's gamepad, or unplugs it
// without one. Returns the source the Wii Remote takes.
static std::optional<WiimoteSource> ApplyWiimotePlayer(int port, const ciface::Core::Device* device)
{
  if (Config::Get(Config::GetInfoForWiimoteSource(port)) == WiimoteSource::Real)
    return std::nullopt;

  auto* wiimote = static_cast<WiimoteEmu::Wiimote*>(Wiimote::GetConfig()->GetController(port));
  if (device)
  {
    wiimote->LoadGamepadDefaults(g_controller_interface, *device);
    wiimote->UpdateReferences(g_controller_interface);
  }
  return device ? WiimoteSource::Emulated : WiimoteSource::None;
}

// Maps the port to its player's gamepad. A free port reads as unplugged. Returns the source the
// Wii Remote of the same number takes.
static std::optional<WiimoteSource> ApplyPlayer(int port)
{
  const std::string& gamepad = s_players.GetGamepad(port);
  const std::optional<int> chan = GetAdapterChannel(gamepad);
  s_adapter_channel[port] = chan.value_or(-1);
  s_adapter_send_origin[port] = chan.has_value();

  std::shared_ptr<ciface::Core::Device> device;
  if (!gamepad.empty() && !chan)
  {
    ciface::Core::DeviceQualifier qualifier;
    qualifier.FromString(gamepad);
    device = g_controller_interface.FindDevice(qualifier);
  }

  // In a Wii game the player holds a Wii Remote only, so as to join once, unless the game is set
  // to play with a GameCube controller or the player holds one on the adapter.
  const bool is_wii = Core::System::GetInstance().IsWii();
  const bool gamecube_controller = !is_wii || chan || GameTakesNoWiimote(port);
  const ciface::Core::Device* pad_device = gamecube_controller ? device.get() : nullptr;

  auto* pad = static_cast<GCPad*>(s_config.GetController(port));
  // Keep a mapping made for a gamepad of the same kind (same source and name, another number).
  if (pad_device && HasInputMapped(*pad) &&
      InputCommon::IsSameKindOfGamepad(pad->GetDefaultDevice().ToString(), gamepad))
  {
    pad->SetDefaultDevice(gamepad);
  }
  else if (pad_device)
  {
    pad->LoadGamepadDefaults(g_controller_interface, *pad_device);
  }
  else
  {
    pad->SetDefaultDevice(ciface::Core::DeviceQualifier{});
  }
  pad->UpdateReferences(g_controller_interface);

  std::optional<WiimoteSource> wiimote_source;
  if (is_wii)
    wiimote_source = ApplyWiimotePlayer(port, gamecube_controller ? nullptr : device.get());

  if (gamepad.empty())
  {
    INFO_LOG_FMT(SERIALINTERFACE, "Player {} is free", port + 1);
    return wiimote_source;
  }

  // Plug in a controller for this game only, like on the console.
  if (gamecube_controller &&
      Config::Get(Config::GetInfoForSIDevice(port)) == SerialInterface::SIDEVICE_NONE)
  {
    Config::SetCurrent(Config::GetInfoForSIDevice(port), SerialInterface::SIDEVICE_GC_CONTROLLER);
  }

  const std::string name = device ? device->GetName() : "GameCube Controller";
  INFO_LOG_FMT(SERIALINTERFACE, "Gamepad {} is player {}", gamepad, port + 1);
  OSD::AddMessage(fmt::format("Player {}: {}", port + 1, name), OSD::Duration::NORMAL);
  return wiimote_source;
}

static void ApplyChangedPlayers()
{
  if (!s_changed_ports)
    return;
  std::array<std::optional<WiimoteSource>, PORT_COUNT> wiimote_sources;
  {
    const auto lock = ControllerEmu::EmulatedController::GetStateLock();
    for (int port = 0; port < PORT_COUNT; ++port)
    {
      if (s_changed_ports & (1 << port))
        wiimote_sources[port] = ApplyPlayer(port);
    }
  }
  s_changed_ports = 0;

  // Out of the controllers' lock: a Wii Remote that changes source populates the devices again.
  for (int port = 0; port < PORT_COUNT; ++port)
  {
    if (wiimote_sources[port])
      Config::SetCurrent(Config::GetInfoForWiimoteSource(port), *wiimote_sources[port]);
  }
}

// Maps every port again, a free one as unplugged.
static void ApplyAllPlayers()
{
  const auto ports = GetPorts();
  for (int port = 0; port < PORT_COUNT; ++port)
  {
    if (ports[port])
      s_changed_ports |= 1 << port;
  }
  ApplyChangedPlayers();
}

static void RefreshPlayers(bool undocked)
{
  for (const int port : s_players.Refresh(GetConnectedGamepads(), GetPorts(), undocked))
    s_changed_ports |= 1 << port;
}

// A gamepad that plays as no one joins when one of its buttons goes down.
static bool OnGamepadButtonPress(const ciface::Core::Device& device)
{
  if (!device.GetPreferredId())
    return false;
  ciface::Core::DeviceQualifier qualifier;
  qualifier.FromDevice(&device);

  std::lock_guard lk(s_players_mutex);
  if (!s_players_enabled)
    return false;
  const std::optional<int> port = s_players.Join(qualifier.ToString(), GetPorts());
  if (!port)
    return false;
  s_changed_ports |= 1 << *port;
  return true;
}

void AutoAssignGamepads()
{
  ciface::Android::SetButtonPressCallback(OnGamepadButtonPress);

  std::lock_guard lk(s_players_mutex);
  s_players_enabled = true;

  // Players keep their gamepad from a game to the next, and every other port reads as unplugged.
  RefreshPlayers(false);
  ApplyAllPlayers();
}

void ApplyPlayers()
{
  std::lock_guard lk(s_players_mutex);
  if (s_players_enabled)
    ApplyAllPlayers();
}

void HandBackFirstPlayer()
{
  s_undocked = true;
}

static void OnDevicesChanged()
{
  std::lock_guard lk(s_hotplug_mutex);
  s_hotplug.DevicesChanged(InputCommon::GamepadHotplug::Clock::now());
}

// Done from the controller polling, so only while a game runs: frees the players whose gamepad
// went away once the devices have settled, and lets the adapter controllers join on a button press.
void UpdatePlayers()
{
  std::lock_guard lk(s_players_mutex);
  if (!s_players_enabled)
    return;

  u8 adapter_connected = 0;
  u8 adapter_pressed = 0;
  if (GCAdapter::IsDetected(nullptr))
  {
    for (int chan = 0; chan < PORT_COUNT; ++chan)
    {
      if (!GCAdapter::DeviceConnected(chan))
        continue;
      adapter_connected |= 1 << chan;
      if (GCAdapter::PeekInput(chan).button & ADAPTER_BUTTONS)
        adapter_pressed |= 1 << chan;
    }
  }

  bool settled;
  {
    std::lock_guard hotplug_lk(s_hotplug_mutex);
    settled = s_hotplug.TakeSettled(InputCommon::GamepadHotplug::Clock::now());
  }
  const bool undocked = s_undocked.exchange(false);
  const bool adapter_changed = adapter_connected != s_adapter_connected;
  s_adapter_connected = adapter_connected;
  if (settled || adapter_changed || undocked)
  {
    RefreshPlayers(undocked);
  }

  const u8 new_presses = adapter_pressed & ~s_adapter_pressed;
  s_adapter_pressed = adapter_pressed;
  if (new_presses)
  {
    const auto ports = GetPorts();
    for (int chan = 0; chan < PORT_COUNT; ++chan)
    {
      if (!(new_presses & (1 << chan)))
        continue;
      if (const std::optional<int> port = s_players.Join(AdapterGamepad(chan), ports))
        s_changed_ports |= 1 << *port;
    }
  }

  ApplyChangedPlayers();
}
#else
void AutoAssignGamepads()
{
}

void HandBackFirstPlayer()
{
}

void UpdatePlayers()
{
}

void ApplyPlayers()
{
}
#endif

void Shutdown()
{
#ifdef ANDROID
  ciface::Android::SetButtonPressCallback(nullptr);
  {
    std::lock_guard lk(s_players_mutex);
    s_players_enabled = false;
  }
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
#ifdef ANDROID
  UpdatePlayers();
  if (const int chan = s_adapter_channel[pad_num]; chan >= 0)
  {
    return s_adapter_send_origin[pad_num].exchange(false) ? GCAdapter::GetOrigin(chan) :
                                                            GCAdapter::PeekInput(chan);
  }
#endif
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetInput();
}

ControllerEmu::ControlGroup* GetGroup(int pad_num, PadGroup group)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetGroup(group);
}

void Rumble(const int pad_num, const ControlState strength)
{
#ifdef ANDROID
  if (const int chan = s_adapter_channel[pad_num]; chan >= 0)
  {
    GCAdapter::Output(chan, static_cast<u8>(strength));
    return;
  }
#endif
  static_cast<GCPad*>(s_config.GetController(pad_num))->SetOutput(strength);
}

void ResetRumble(const int pad_num)
{
#ifdef ANDROID
  if (const int chan = s_adapter_channel[pad_num]; chan >= 0)
  {
    GCAdapter::Output(chan, 0);
    return;
  }
#endif
  static_cast<GCPad*>(s_config.GetController(pad_num))->SetOutput(0.0);
}

bool GetMicButton(const int pad_num)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetMicButton();
}
}  // namespace Pad

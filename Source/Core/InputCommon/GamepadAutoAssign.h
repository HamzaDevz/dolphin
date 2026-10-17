// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace InputCommon
{
// One emulated controller port, seen by the gamepad auto-assignment.
struct GamepadPort
{
  // The port takes a controller (for example its SI device is a GameCube controller).
  bool active = false;
  // At least one of its buttons or sticks has an expression.
  bool mapped = false;
  // Its default device, as a qualifier string ("Android/2/Wireless Controller").
  std::string device;
  // The port has no controller plugged in but can take one.
  bool pluggable = false;
};

struct GamepadAssignment
{
  int port;
  // The gamepad given to the port.
  std::string device;
  // The port was mapped for a gamepad of the same kind (same source and name, another number):
  // keep its expressions and only retarget them.
  bool keep_mapping;
};

// Hands connected gamepads to the active ports that have none, the way a console does when a
// controller is plugged in. A port keeps its device when it is mapped, its device is connected and
// no lower port already uses that device; every other active port, and every pluggable port, takes
// the next free gamepad, in player order.
//
// connected: every connected device. gamepads: the connected gamepads, in player order.
// new_gamepads: when not null, only these (just connected) gamepads may be handed out, so that a
// gamepad that briefly disappears does not lose its port to another one.
std::vector<GamepadAssignment> PlanGamepadAssignment(const std::vector<GamepadPort>& ports,
                                                     const std::vector<std::string>& connected,
                                                     const std::vector<std::string>& gamepads,
                                                     const std::vector<std::string>* new_gamepads);

// Finds the gamepads connected during a game once the devices have stopped changing for a moment:
// the Odin 3 hides every external gamepad behind its own copy right after it connects, and only
// the copy must become a player.
class GamepadHotplug
{
public:
  using Clock = std::chrono::steady_clock;
  static constexpr auto SETTLE_TIME = std::chrono::milliseconds(500);

  // gamepads: the gamepads that already have their place.
  void Reset(std::vector<std::string> gamepads);
  void DevicesChanged(Clock::time_point now);
  bool IsSettled(Clock::time_point now) const;
  // The gamepads that were not there before, in player order.
  std::vector<std::string> TakeNewGamepads(const std::vector<std::string>& gamepads);

private:
  std::vector<std::string> m_known;
  std::optional<Clock::time_point> m_changed_at;
};
}  // namespace InputCommon

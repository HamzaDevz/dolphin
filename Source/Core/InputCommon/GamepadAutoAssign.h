// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
}  // namespace InputCommon

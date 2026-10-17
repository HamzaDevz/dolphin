// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "InputCommon/GamepadAutoAssign.h"

#include <algorithm>
#include <string_view>

namespace InputCommon
{
namespace
{
bool Contains(const std::vector<std::string>& list, const std::string& value)
{
  return std::ranges::find(list, value) != list.end();
}

// "Android/2/Wireless Controller" and "Android/3/Wireless Controller" are the same kind of gamepad.
bool SameKind(std::string_view a, std::string_view b)
{
  const auto source_a = a.find('/'), source_b = b.find('/');
  if (source_a == std::string_view::npos || source_b == std::string_view::npos)
    return false;
  const auto name_a = a.find('/', source_a + 1), name_b = b.find('/', source_b + 1);
  if (name_a == std::string_view::npos || name_b == std::string_view::npos)
    return false;
  return a.substr(0, source_a) == b.substr(0, source_b) && a.substr(name_a) == b.substr(name_b);
}
}  // namespace

std::vector<GamepadAssignment> PlanGamepadAssignment(const std::vector<GamepadPort>& ports,
                                                     const std::vector<std::string>& connected,
                                                     const std::vector<std::string>& gamepads,
                                                     const std::vector<std::string>* new_gamepads)
{
  std::vector<std::string> used;
  std::vector<int> without_device;
  for (int i = 0; i < static_cast<int>(ports.size()); ++i)
  {
    const GamepadPort& port = ports[i];
    if (!port.active)
    {
      if (port.pluggable)
        without_device.push_back(i);
      continue;
    }
    if (port.mapped && Contains(connected, port.device) && !Contains(used, port.device))
      used.push_back(port.device);
    else
      without_device.push_back(i);
  }

  std::vector<GamepadAssignment> plan;
  auto port = without_device.begin();
  for (const std::string& gamepad : gamepads)
  {
    if (port == without_device.end())
      break;
    if (Contains(used, gamepad) || (new_gamepads && !Contains(*new_gamepads, gamepad)))
      continue;
    const GamepadPort& p = ports[*port];
    plan.push_back({*port, gamepad, p.mapped && SameKind(p.device, gamepad)});
    used.push_back(gamepad);
    ++port;
  }
  return plan;
}
}  // namespace InputCommon

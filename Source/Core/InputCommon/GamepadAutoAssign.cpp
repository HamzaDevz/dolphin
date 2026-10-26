// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "InputCommon/GamepadAutoAssign.h"

#include <algorithm>
#include <utility>

#include <fmt/format.h>

#include "InputCommon/ControllerInterface/CoreDevice.h"

namespace InputCommon
{
namespace
{
bool Contains(const std::vector<std::string>& list, const std::string& value)
{
  return std::ranges::find(list, value) != list.end();
}

// The source and the name of "Source/Id/Name", without the id.
std::optional<std::pair<std::string_view, std::string_view>> SplitQualifier(std::string_view gamepad)
{
  const auto source_end = gamepad.find('/');
  if (source_end == std::string_view::npos)
    return std::nullopt;
  const auto id_end = gamepad.find('/', source_end + 1);
  if (id_end == std::string_view::npos)
    return std::nullopt;
  return std::make_pair(gamepad.substr(0, source_end), gamepad.substr(id_end + 1));
}
}  // namespace

bool IsBuiltInGamepad(std::string_view gamepad)
{
  const auto parts = SplitQualifier(gamepad);
  return parts && parts->first == "Android" && parts->second == BUILT_IN_GAMEPAD;
}

bool IsSameKindOfGamepad(std::string_view a, std::string_view b)
{
  const auto parts_a = SplitQualifier(a);
  return parts_a && parts_a == SplitQualifier(b);
}

std::string AnyInputOf(const ciface::Core::Device& device,
                       std::initializer_list<std::string_view> names)
{
  std::string expression;
  for (const std::string_view name : names)
  {
    if (!device.FindInput(name))
      continue;
    if (!expression.empty())
      expression += " | ";
    expression += fmt::format("`{}`", name);
  }
  return expression;
}

GamepadPlayers::GamepadPlayers(int port_count) : m_gamepads(port_count)
{
}

std::optional<int> GamepadPlayers::Join(const std::string& gamepad, const std::vector<bool>& ports)
{
  if (Contains(m_gamepads, gamepad))
    return std::nullopt;

  const int count = static_cast<int>(m_gamepads.size());
  const bool built_in = IsBuiltInGamepad(gamepad);
  std::optional<int> port;
  if (built_in && ports[0] &&
      std::all_of(m_gamepads.begin() + 1, m_gamepads.end(), [](const auto& g) { return g.empty(); }))
  {
    port = 0;
  }
  else
  {
    for (int i = 0; i < count; ++i)
    {
      const std::string& holder = m_gamepads[i];
      if (ports[i] && (holder.empty() || (IsBuiltInGamepad(holder) && !built_in)))
      {
        port = i;
        break;
      }
    }
  }

  if (port)
    m_gamepads[*port] = gamepad;
  return port;
}

std::vector<int> GamepadPlayers::Refresh(const std::vector<std::string>& connected,
                                         const std::vector<bool>& ports, bool undocked)
{
  std::vector<int> changed;
  for (int i = 0; i < static_cast<int>(m_gamepads.size()); ++i)
  {
    if (!m_gamepads[i].empty() && (!ports[i] || !Contains(connected, m_gamepads[i])))
    {
      m_gamepads[i].clear();
      changed.push_back(i);
    }
  }

  const auto built_in = std::ranges::find_if(connected, IsBuiltInGamepad);
  if (ports[0] && built_in != connected.end() && (undocked || m_gamepads[0].empty()) &&
      m_gamepads[0] != *built_in)
  {
    const auto other = std::ranges::find(m_gamepads, *built_in);
    if (other != m_gamepads.end())
    {
      other->clear();
      changed.push_back(static_cast<int>(other - m_gamepads.begin()));
    }
    m_gamepads[0] = *built_in;
    if (std::ranges::find(changed, 0) == changed.end())
      changed.push_back(0);
  }
  return changed;
}

const std::string& GamepadPlayers::GetGamepad(int port) const
{
  return m_gamepads[port];
}

void GamepadHotplug::DevicesChanged(Clock::time_point now)
{
  m_changed_at = now;
}

bool GamepadHotplug::TakeSettled(Clock::time_point now)
{
  if (!m_changed_at || now - *m_changed_at < SETTLE_TIME)
    return false;
  m_changed_at.reset();
  return true;
}
}  // namespace InputCommon

// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace InputCommon
{
// The handheld's own gamepad, which gives player 1 up to any gamepad that joins.
constexpr std::string_view BUILT_IN_GAMEPAD = "Odin Controller";

// Gamepads are named by their device qualifier ("Android/2/Wireless Controller").
bool IsBuiltInGamepad(std::string_view gamepad);
// "Android/2/Wireless Controller" and "Android/3/Wireless Controller" are the same kind of gamepad.
bool IsSameKindOfGamepad(std::string_view a, std::string_view b);

// Hands the emulated controller ports to gamepads the way a Switch does: a gamepad that plays as
// no one joins on a button press, as player 1 if the handheld's own gamepad or no gamepad holds
// it, else as the first free player. The handheld's own gamepad takes player 1 back when no one
// else plays.
class GamepadPlayers
{
public:
  explicit GamepadPlayers(int port_count);

  // ports: whether each port can take a gamepad. Returns the port the gamepad joins, if any.
  std::optional<int> Join(const std::string& gamepad, const std::vector<bool>& ports);

  // Frees the ports whose gamepad went away, and gives player 1 to the handheld's own gamepad when
  // its gamepad went away too or when the handheld leaves its dock. Returns the ports that changed.
  std::vector<int> Refresh(const std::vector<std::string>& connected,
                           const std::vector<bool>& ports, bool undocked);

  // The gamepad playing on the port, empty when the port is free.
  const std::string& GetGamepad(int port) const;

private:
  std::vector<std::string> m_gamepads;
};

// Tells when the devices have stopped changing for a moment: the Odin 3 hides every external
// gamepad behind its own copy right after it connects.
class GamepadHotplug
{
public:
  using Clock = std::chrono::steady_clock;
  static constexpr auto SETTLE_TIME = std::chrono::milliseconds(500);

  void DevicesChanged(Clock::time_point now);
  // True once the devices have settled after a change, then false until the next change.
  bool TakeSettled(Clock::time_point now);

private:
  std::optional<Clock::time_point> m_changed_at;
};
}  // namespace InputCommon

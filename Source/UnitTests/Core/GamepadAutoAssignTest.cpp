// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "InputCommon/ControllerInterface/CoreDevice.h"
#include "InputCommon/GamepadAutoAssign.h"

using InputCommon::GamepadHotplug;
using InputCommon::GamepadPlayers;

namespace
{
const std::string ODIN = "Android/1/Odin Controller";
const std::string XBOX = "Android/2/Xbox Wireless Controller";
const std::string DS4 = "Android/3/Wireless Controller";
const std::string LIC = "Android/4/Lic Pro Controller";
const std::string ADAPTER = "GCAdapter/0/GameCube Controller";
const std::vector<bool> ALL_PORTS = {true, true, true, true};

std::vector<std::string> Players(const GamepadPlayers& players)
{
  return {players.GetGamepad(0), players.GetGamepad(1), players.GetGamepad(2),
          players.GetGamepad(3)};
}

class FakeGamepad final : public ciface::Core::Device
{
public:
  class FakeInput final : public Input
  {
  public:
    explicit FakeInput(std::string name) : m_name(std::move(name)) {}
    std::string GetName() const override { return m_name; }
    ControlState GetState() const override { return 0; }

  private:
    std::string m_name;
  };

  explicit FakeGamepad(std::initializer_list<std::string> inputs)
  {
    for (const std::string& input : inputs)
      AddInput(new FakeInput(input));
  }
  std::string GetName() const override { return "Xbox Wireless Controller"; }
  std::string GetSource() const override { return "Android"; }
};

// The Odin alone, as when a game starts.
GamepadPlayers OdinAlone()
{
  GamepadPlayers players(4);
  players.Refresh({ODIN}, ALL_PORTS, false);
  return players;
}
}  // namespace

TEST(GamepadPlayers, TheOdinAloneIsPlayerOne)
{
  const GamepadPlayers players = OdinAlone();
  EXPECT_EQ(Players(players), (std::vector<std::string>{ODIN, "", "", ""}));
}

TEST(GamepadPlayers, AGamepadTakesPlayerOneFromTheOdin)
{
  GamepadPlayers players = OdinAlone();
  EXPECT_EQ(players.Join(XBOX, ALL_PORTS), 0);
  EXPECT_EQ(Players(players), (std::vector<std::string>{XBOX, "", "", ""}));
}

TEST(GamepadPlayers, TheNextGamepadsTakeTheNextPlayers)
{
  GamepadPlayers players = OdinAlone();
  EXPECT_EQ(players.Join(XBOX, ALL_PORTS), 0);
  EXPECT_EQ(players.Join(DS4, ALL_PORTS), 1);
  EXPECT_EQ(players.Join(ADAPTER, ALL_PORTS), 2);
  EXPECT_EQ(players.Join(LIC, ALL_PORTS), 3);
  EXPECT_EQ(players.Join("Android/5/Other", ALL_PORTS), std::nullopt);
}

TEST(GamepadPlayers, AGamepadThatPlaysDoesNotMove)
{
  GamepadPlayers players = OdinAlone();
  EXPECT_EQ(players.Join(ODIN, ALL_PORTS), std::nullopt);
  players.Join(XBOX, ALL_PORTS);
  EXPECT_EQ(players.Join(XBOX, ALL_PORTS), std::nullopt);
  EXPECT_EQ(Players(players), (std::vector<std::string>{XBOX, "", "", ""}));
}

TEST(GamepadPlayers, TheOdinTakesPlayerOneBackWhenNoOneElsePlays)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  EXPECT_EQ(players.Join(ODIN, ALL_PORTS), 0);
  EXPECT_EQ(Players(players), (std::vector<std::string>{ODIN, "", "", ""}));
}

TEST(GamepadPlayers, TheOdinJoinsAfterTheOthers)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  players.Join(DS4, ALL_PORTS);
  EXPECT_EQ(players.Join(ODIN, ALL_PORTS), 2);
}

TEST(GamepadPlayers, AGamepadTakesTheOdinsPlayerAfterTheOthers)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  players.Join(DS4, ALL_PORTS);
  players.Join(ODIN, ALL_PORTS);
  EXPECT_EQ(players.Join(ADAPTER, ALL_PORTS), 2);
  EXPECT_EQ(players.Join(LIC, ALL_PORTS), 3);
  EXPECT_EQ(Players(players), (std::vector<std::string>{XBOX, DS4, ADAPTER, LIC}));
}

TEST(GamepadPlayers, SkipsPortsThatTakeNoGamepad)
{
  GamepadPlayers players(4);
  const std::vector<bool> ports = {false, true, false, true};
  EXPECT_EQ(players.Join(XBOX, ports), 1);
  EXPECT_EQ(players.Join(DS4, ports), 3);
  EXPECT_EQ(players.Join(ODIN, ports), std::nullopt);
}

TEST(GamepadPlayers, FreesThePlayersWhoseGamepadWentAway)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  players.Join(DS4, ALL_PORTS);
  players.Join(ADAPTER, ALL_PORTS);
  EXPECT_EQ(players.Refresh({ODIN, XBOX, ADAPTER}, ALL_PORTS, false), (std::vector<int>{1}));
  EXPECT_EQ(Players(players), (std::vector<std::string>{XBOX, "", ADAPTER, ""}));
  EXPECT_EQ(players.Join(DS4, ALL_PORTS), 1);
}

TEST(GamepadPlayers, TheOdinTakesPlayerOneWhenItsGamepadWentAway)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  players.Join(DS4, ALL_PORTS);
  players.Join(ODIN, ALL_PORTS);
  EXPECT_EQ(players.Refresh({ODIN, DS4}, ALL_PORTS, false), (std::vector<int>{0, 2}));
  EXPECT_EQ(Players(players), (std::vector<std::string>{ODIN, DS4, "", ""}));
}

TEST(GamepadPlayers, TheOdinTakesPlayerOneWhenItLeavesTheDock)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  players.Join(DS4, ALL_PORTS);
  EXPECT_EQ(players.Refresh({ODIN, XBOX, DS4}, ALL_PORTS, false), (std::vector<int>{}));
  EXPECT_EQ(players.Refresh({ODIN, XBOX, DS4}, ALL_PORTS, true), (std::vector<int>{0}));
  EXPECT_EQ(Players(players), (std::vector<std::string>{ODIN, DS4, "", ""}));
}

TEST(GamepadPlayers, FreesThePortsThatNoLongerTakeAGamepad)
{
  GamepadPlayers players = OdinAlone();
  players.Join(XBOX, ALL_PORTS);
  players.Join(DS4, ALL_PORTS);
  EXPECT_EQ(players.Refresh({ODIN, XBOX, DS4}, {true, false, true, true}, false),
            (std::vector<int>{1}));
}

TEST(GamepadPlayers, NoOneIsPlayerOneWithoutTheOdin)
{
  GamepadPlayers players(4);
  EXPECT_EQ(players.Refresh({XBOX}, ALL_PORTS, false), (std::vector<int>{}));
  EXPECT_EQ(players.Join(XBOX, ALL_PORTS), 0);
}

TEST(GamepadPlayers, TellsGamepadsApart)
{
  EXPECT_TRUE(InputCommon::IsBuiltInGamepad(ODIN));
  EXPECT_TRUE(InputCommon::IsBuiltInGamepad("Android/0/Odin Controller"));
  EXPECT_FALSE(InputCommon::IsBuiltInGamepad(XBOX));
  EXPECT_FALSE(InputCommon::IsBuiltInGamepad("Odin Controller"));
  EXPECT_TRUE(InputCommon::IsSameKindOfGamepad(DS4, "Android/7/Wireless Controller"));
  EXPECT_FALSE(InputCommon::IsSameKindOfGamepad(DS4, XBOX));
  EXPECT_FALSE(InputCommon::IsSameKindOfGamepad("", ""));
}

TEST(GamepadPlayers, MapsTheInputsTheGamepadHas)
{
  const FakeGamepad gamepad{"Button A", "Axis 22+"};
  EXPECT_EQ(InputCommon::AnyInputOf(gamepad, {"Axis 18+", "Axis 22+", "Button A"}),
            "`Axis 22+` | `Button A`");
  EXPECT_EQ(InputCommon::AnyInputOf(gamepad, {"Button L3"}), "");
}

TEST(GamepadHotplug, SettlesOnceAfterTheDevicesStopChanging)
{
  using namespace std::chrono_literals;
  GamepadHotplug hotplug;
  const auto t = GamepadHotplug::Clock::now();
  EXPECT_FALSE(hotplug.TakeSettled(t));
  hotplug.DevicesChanged(t);
  hotplug.DevicesChanged(t + 90ms);
  EXPECT_FALSE(hotplug.TakeSettled(t + 500ms));
  EXPECT_TRUE(hotplug.TakeSettled(t + 590ms));
  EXPECT_FALSE(hotplug.TakeSettled(t + 1s));
}

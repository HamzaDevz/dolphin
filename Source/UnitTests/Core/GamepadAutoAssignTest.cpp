// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "InputCommon/GamepadAutoAssign.h"

using InputCommon::GamepadAssignment;
using InputCommon::GamepadHotplug;
using InputCommon::GamepadPort;
using InputCommon::PlanGamepadAssignment;

namespace
{
const std::string ODIN = "Android/1/Odin Controller";
const std::string PAD2 = "Android/2/Wireless Controller";
const std::string PAD3 = "Android/3/Wireless Controller";
const std::string XBOX3 = "Android/3/Xbox Wireless Controller";
const std::string SENSORS = "Android/0/Device Sensors";
const std::string KEYBOARD = "Android/0/Keyboard";

std::vector<GamepadAssignment> Plan(const std::vector<GamepadPort>& ports,
                                    const std::vector<std::string>& gamepads,
                                    const std::vector<std::string>* new_gamepads = nullptr)
{
  std::vector<std::string> connected = gamepads;
  connected.push_back(SENSORS);
  return PlanGamepadAssignment(ports, connected, gamepads, new_gamepads);
}

void ExpectAssigned(const GamepadAssignment& a, int port, const std::string& device, bool keep)
{
  EXPECT_EQ(a.port, port);
  EXPECT_EQ(a.device, device);
  EXPECT_EQ(a.keep_mapping, keep);
}
}  // namespace

// The Odin 3 config found on the console: port 1 mapped to the built-in controller, port 2 points to
// a PS4 pad but has no binding, port 3 points to the Odin with no binding, port 4 is unplugged.
TEST(GamepadAutoAssign, FillsEmptyPortsWithFreeGamepads)
{
  const auto plan = Plan({{true, true, ODIN}, {true, false, PAD2}, {true, false, ODIN}, {false, false, ODIN}},
                         {ODIN, PAD2, PAD3});
  ASSERT_EQ(plan.size(), 2u);
  ExpectAssigned(plan[0], 1, PAD2, false);
  ExpectAssigned(plan[1], 2, PAD3, false);
}

TEST(GamepadAutoAssign, KeepsMappedConnectedPorts)
{
  EXPECT_TRUE(Plan({{true, true, ODIN}, {true, true, PAD2}}, {ODIN, PAD2, PAD3}).empty());
}

// A game profile that maps port 2 to the same pad as port 1 (Mario Kart Wii on the console).
TEST(GamepadAutoAssign, MovesADuplicatePortToAFreeGamepad)
{
  const auto plan = Plan({{true, true, ODIN}, {true, true, ODIN}}, {ODIN, PAD2});
  ASSERT_EQ(plan.size(), 1u);
  ExpectAssigned(plan[0], 1, PAD2, false);
}

TEST(GamepadAutoAssign, LeavesADuplicateAloneWithoutAFreeGamepad)
{
  EXPECT_TRUE(Plan({{true, true, ODIN}, {true, true, ODIN}}, {ODIN}).empty());
}

// The PS4 pad mapped as "2" came back as "3": keep its custom mapping.
TEST(GamepadAutoAssign, RetargetsAMappingToTheSameKindOfGamepad)
{
  const auto plan = Plan({{true, true, ODIN}, {true, true, PAD2}}, {ODIN, PAD3});
  ASSERT_EQ(plan.size(), 1u);
  ExpectAssigned(plan[0], 1, PAD3, true);
}

TEST(GamepadAutoAssign, RemapsForAnotherKindOfGamepad)
{
  const auto plan = Plan({{true, true, ODIN}, {true, true, PAD2}}, {ODIN, XBOX3});
  ASSERT_EQ(plan.size(), 1u);
  ExpectAssigned(plan[0], 1, XBOX3, false);
}

TEST(GamepadAutoAssign, IgnoresInactivePorts)
{
  EXPECT_TRUE(Plan({{true, true, ODIN}, {false, false, ""}}, {ODIN, PAD2}).empty());
}

// A game setting that unplugs port 2 (Mario Kart Wii on the console) must not leave a gamepad out.
TEST(GamepadAutoAssign, PlugsEmptyPortsForExtraGamepads)
{
  const auto plan = Plan({{true, true, ODIN},
                          {false, false, ODIN, true},
                          {false, false, "", true},
                          {false, false, "", false}},
                         {ODIN, PAD2, PAD3, XBOX3});
  ASSERT_EQ(plan.size(), 2u);
  ExpectAssigned(plan[0], 1, PAD2, false);
  ExpectAssigned(plan[1], 2, PAD3, false);
}

TEST(GamepadAutoAssign, FillsPortsInPlayerOrder)
{
  const auto plan =
      Plan({{true, true, ODIN}, {false, false, "", true}, {true, false, ""}}, {ODIN, PAD2, PAD3});
  ASSERT_EQ(plan.size(), 2u);
  ExpectAssigned(plan[0], 1, PAD2, false);
  ExpectAssigned(plan[1], 2, PAD3, false);
}

TEST(GamepadAutoAssign, KeepsAPortMappedToAConnectedNonGamepad)
{
  std::vector<std::string> connected = {ODIN, PAD2, KEYBOARD};
  const std::vector<std::string> gamepads = {ODIN, PAD2};
  EXPECT_TRUE(
      PlanGamepadAssignment({{true, true, ODIN}, {true, true, KEYBOARD}}, connected, gamepads, nullptr)
          .empty());
}

TEST(GamepadAutoAssign, FillsAnUnmappedPortWithNoDevice)
{
  const auto plan = Plan({{true, true, ODIN}, {true, false, ""}}, {ODIN, PAD2});
  ASSERT_EQ(plan.size(), 1u);
  ExpectAssigned(plan[0], 1, PAD2, false);
}

// On hotplug only the gamepads that just appeared are handed out: a pad that reconnects
// (Android removes and re-adds it) must not steal the port of another pad that is briefly away.
TEST(GamepadAutoAssign, HotplugOnlyHandsOutNewGamepads)
{
  const std::vector<GamepadPort> ports = {{true, true, ODIN}, {true, true, PAD2}, {true, false, ""}};
  const std::vector<std::string> none;
  EXPECT_TRUE(Plan(ports, {ODIN, PAD3}, &none).empty());

  const std::vector<std::string> pad3 = {PAD3};
  const auto plan = Plan(ports, {ODIN, PAD3}, &pad3);
  ASSERT_EQ(plan.size(), 1u);
  ExpectAssigned(plan[0], 1, PAD3, true);
}

TEST(GamepadAutoAssign, NeverGivesOneGamepadToTwoPorts)
{
  const auto plan = Plan({{true, false, ""}, {true, false, ""}, {true, false, ""}}, {ODIN, PAD2});
  ASSERT_EQ(plan.size(), 2u);
  ExpectAssigned(plan[0], 0, ODIN, false);
  ExpectAssigned(plan[1], 1, PAD2, false);
}

namespace
{
using namespace std::chrono_literals;
const GamepadHotplug::Clock::time_point T0{};
}  // namespace

TEST(GamepadHotplug, WaitsForTheDevicesToSettle)
{
  GamepadHotplug hotplug;
  hotplug.Reset({ODIN});
  EXPECT_FALSE(hotplug.IsSettled(T0));

  hotplug.DevicesChanged(T0);
  EXPECT_FALSE(hotplug.IsSettled(T0 + 100ms));
  EXPECT_TRUE(hotplug.IsSettled(T0 + GamepadHotplug::SETTLE_TIME));
  EXPECT_EQ(hotplug.TakeNewGamepads({ODIN, PAD2}), std::vector<std::string>{PAD2});
  EXPECT_FALSE(hotplug.IsSettled(T0 + 1s));
}

// The Odin 3 hides every external gamepad behind its own copy, 85 ms after it connects: only the
// copy is a new player.
TEST(GamepadHotplug, SkipsAGamepadReplacedRightAway)
{
  GamepadHotplug hotplug;
  hotplug.Reset({ODIN});
  hotplug.DevicesChanged(T0);         // the gamepad
  hotplug.DevicesChanged(T0 + 85ms);  // the Odin's copy
  hotplug.DevicesChanged(T0 + 88ms);  // the gamepad, hidden
  EXPECT_FALSE(hotplug.IsSettled(T0 + GamepadHotplug::SETTLE_TIME));
  EXPECT_TRUE(hotplug.IsSettled(T0 + 88ms + GamepadHotplug::SETTLE_TIME));
  EXPECT_EQ(hotplug.TakeNewGamepads({ODIN, PAD3}), std::vector<std::string>{PAD3});
}

TEST(GamepadHotplug, AReconnectedGamepadIsNotNew)
{
  GamepadHotplug hotplug;
  hotplug.Reset({ODIN, PAD2});
  hotplug.DevicesChanged(T0);
  hotplug.DevicesChanged(T0 + 50ms);
  EXPECT_TRUE(hotplug.IsSettled(T0 + 1s));
  EXPECT_TRUE(hotplug.TakeNewGamepads({ODIN, PAD2}).empty());
}

TEST(GamepadHotplug, AGamepadBackAfterLeavingIsNew)
{
  GamepadHotplug hotplug;
  hotplug.Reset({ODIN, PAD2});
  hotplug.DevicesChanged(T0);
  EXPECT_TRUE(hotplug.TakeNewGamepads({ODIN}).empty());
  hotplug.DevicesChanged(T0 + 5s);
  EXPECT_EQ(hotplug.TakeNewGamepads({ODIN, PAD2}), std::vector<std::string>{PAD2});
}

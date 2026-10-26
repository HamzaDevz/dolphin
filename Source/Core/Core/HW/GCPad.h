// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"
#include "InputCommon/ControllerInterface/CoreDevice.h"

class InputConfig;
enum class PadGroup;
struct GCPadStatus;

namespace ControllerEmu
{
class ControlGroup;
}

namespace Pad
{
void Shutdown();
void Initialize();
void LoadConfig();
// Android: a gamepad that plays as no one joins as a player when one of its buttons goes down,
// like on a Switch. Called when a game starts.
void AutoAssignGamepads();
// Android: gives player 1 back to the handheld's own gamepad, when it leaves its dock.
void HandBackFirstPlayer();
// Android: frees the players whose gamepad went away and maps the ones who joined. Called from
// the controller polling.
void UpdatePlayers();
// Android: maps the players' gamepads again, after loading the controller configs undid them.
void ApplyPlayers();
void GenerateDynamicInputTextures();
bool IsInitialized();

InputConfig* GetConfig();

GCPadStatus GetStatus(int pad_num);
ControllerEmu::ControlGroup* GetGroup(int pad_num, PadGroup group);
void Rumble(int pad_num, ControlState strength);
void ResetRumble(int pad_num);

bool GetMicButton(int pad_num);
}  // namespace Pad

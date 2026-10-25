// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "Common/CommonTypes.h"

struct GCPadStatus;

namespace GCAdapter
{
void Init();
void ResetRumble();
void Shutdown();
void SetAdapterCallback(std::function<void(void)> func);

// Buttons have PAD_GET_ORIGIN set on new connection
// Netplay and CSIDevice_GCAdapter make use of this.
GCPadStatus Input(int chan);
// The state of the controller on the port, without taking the origin of a new connection.
GCPadStatus PeekInput(int chan);
// The state of the controller on the port when it was plugged in, with PAD_GET_ORIGIN set.
GCPadStatus GetOrigin(int chan);

void Output(int chan, u8 rumble_command);
bool IsDetected(const char** error_message);
bool DeviceConnected(int chan);
void ResetDeviceType(int chan);

// Callable from any thread. Returns 0 when the adapter is not detected.
double GetCurrentPollRate();

}  // namespace GCAdapter

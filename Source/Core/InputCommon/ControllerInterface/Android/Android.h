// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "InputCommon/ControllerInterface/CoreDevice.h"
#include "InputCommon/ControllerInterface/InputBackend.h"

namespace ciface::Android
{
std::unique_ptr<ciface::InputBackend> CreateInputBackend(ControllerInterface* controller_interface);

// Called when a button of a gamepad goes down, from the thread that dispatches the key events.
// Returns whether the press is used up and must not reach the app.
void SetButtonPressCallback(std::function<bool(const Core::Device&)> callback);

}  // namespace ciface::Android

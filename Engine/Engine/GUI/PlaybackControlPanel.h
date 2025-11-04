//-----------------------------------------------------------------------------------
// file:    PlaybackControlPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the PlaybackControlPanel ImGui interface for controlling
//          game execution with Play, Pause, and Stop buttons. Integrates with Core's
//          physics_step_mode for pausing game logic.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/*=================================================================================
Function Brief:
Creates and renders an ImGui panel with Play, Pause, and Stop buttons for
controlling game execution. Shows the current playback state and provides
intuitive controls for development and testing.

Where to call it:
Call this function once per frame during the ImGui rendering phase, typically
registered in RegisterAllPanels.h.
==================================================================================*/
void PlaybackControlPanel();

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

namespace Engine {
  namespace PlaybackControl {
    /*=================================================================================
    Function Brief:
    Returns true if the game is currently paused (physics_step_mode is enabled).

    Where to call it:
    Can be called from any system that needs to check if game logic is paused.
    ==================================================================================*/
    bool is_paused();

    /*=================================================================================
    Function Brief:
    Sets the pause state of the game (controls physics_step_mode).

    Parameters:
    paused - true to pause game logic, false to resume

    Where to call it:
    Called internally by PlaybackControlPanel or from other systems that need to
    control game execution state.
    ==================================================================================*/
    void set_paused(bool paused);
  }
}

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

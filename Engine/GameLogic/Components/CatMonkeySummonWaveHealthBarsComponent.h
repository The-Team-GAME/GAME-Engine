//-----------------------------------------------------------------------------------
// file:    CatMonkeySummonWaveHealthBarsComponent.h
// project: Monkey Stole My GPA
//
// brief:   Bottom-screen health bars (enemy-style sprite) for Cat Monkey summon
//          waves. One bar per summon state (Summon1..Summon5).
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

namespace Components
{
  struct CatMonkeySummonWaveHealthBars
  {
    // Layout in screen-space pixels.
    // `center_ui` is the center of the whole 5-bar row.
    Math::Vec2D center_ui = { 800.0f, 860.0f };
    Math::Vec2D bar_size_ui = { 160.0f, 160.0f };
    float spacing_px = 170.0f;

    // Updated by CatMonkeyStateCallbacks.
    // A wave becomes visible after it is spawned.
    // When cleared, it plays a short death animation then hides.
    int wave1_current_hp = 0;
    int wave1_max_hp = 0;
    bool wave1_spawned = false;
    bool wave1_cleared = false;
    float wave1_cleared_time = 0.0f;

    int wave2_current_hp = 0;
    int wave2_max_hp = 0;
    bool wave2_spawned = false;
    bool wave2_cleared = false;
    float wave2_cleared_time = 0.0f;

    int wave3_current_hp = 0;
    int wave3_max_hp = 0;
    bool wave3_spawned = false;
    bool wave3_cleared = false;
    float wave3_cleared_time = 0.0f;

    int wave4_current_hp = 0;
    int wave4_max_hp = 0;
    bool wave4_spawned = false;
    bool wave4_cleared = false;
    float wave4_cleared_time = 0.0f;

    int wave5_current_hp = 0;
    int wave5_max_hp = 0;
    bool wave5_spawned = false;
    bool wave5_cleared = false;
    float wave5_cleared_time = 0.0f;

    bool visible = true;
  };
}

REGISTER_COMPONENT(Components::CatMonkeySummonWaveHealthBars,
  center_ui, bar_size_ui, spacing_px,
  wave1_current_hp, wave1_max_hp, wave1_spawned, wave1_cleared, wave1_cleared_time,
  wave2_current_hp, wave2_max_hp, wave2_spawned, wave2_cleared, wave2_cleared_time,
  wave3_current_hp, wave3_max_hp, wave3_spawned, wave3_cleared, wave3_cleared_time,
  wave4_current_hp, wave4_max_hp, wave4_spawned, wave4_cleared, wave4_cleared_time,
  wave5_current_hp, wave5_max_hp, wave5_spawned, wave5_cleared, wave5_cleared_time,
  visible)

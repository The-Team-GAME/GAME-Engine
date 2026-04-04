//-----------------------------------------------------------------------------------
// file:    CatMonkeySummonHealthBarComponent.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Screen-space health bar for Cat Monkey summons. The bar represents the
//          combined HP pool of the currently-summoned enemies.
//
//// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

namespace Components
{
  struct CatMonkeySummonHealthBar
  {
    // Updated by CatMonkeyStateCallbacks.
    int total_current_hp = 0;
    int total_max_hp = 0;

    // Screen-space layout (pixels). Position is the RECT center.
    Math::Vec2D center_ui = { 800.0f, 860.0f };
    Math::Vec2D size_ui = { 900.0f, 32.0f };

    float border_thickness_px = 4.0f;
    bool visible = true;
  };
}

REGISTER_COMPONENT(Components::CatMonkeySummonHealthBar,
  total_current_hp, total_max_hp,
  center_ui, size_ui,
  border_thickness_px, visible)

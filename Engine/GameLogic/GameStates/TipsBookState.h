//-----------------------------------------------------------------------------------
// file:    TipsBookState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Tips book overlay scene, pushed on top of the active game state.
//          Because it is a separate pushed scene, the underlying state's update()
//          never runs while tips are open — ESC cannot accidentally trigger the
//          pause menu or any other underlying handler.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class TipsBookState : public Engine::Scene
{
  // Layout constants (kept in sync with GameplayState)
  static constexpr float TIPS_BUTTON_HUD_X     = -227.0f;
  static constexpr float TIPS_BUTTON_HUD_Y     = 92.0f;
  static constexpr float TIPS_BUTTON_HUD_SIZE  = 220.0f;
  static constexpr float TIPS_BUTTON_NAV_SIZE  = 84.0f;
  static constexpr float TIPS_OVERLAY_CENTER_X = 800.0f;
  static constexpr float TIPS_OVERLAY_CENTER_Y = 450.0f;
  static constexpr float TIPS_OVERLAY_WIDTH    = 1088.0f;
  static constexpr float TIPS_OVERLAY_HEIGHT   = 768.0f;
  static constexpr float TIPS_NAV_MARGIN_X     = 24.0f;
  static constexpr float TIPS_NAV_MARGIN_Y     = 24.0f;
  static constexpr float TIPS_NAV_OFFSET_Y     = 56.0f;

  Engine::ECS::EntityID bg_id      = 0;
  Engine::ECS::EntityID overlay_id = 0;  // currently visible page entity
  int                   page       = 1;
  bool                  owns_nav_button_ = false;

  // Nav button helpers — reposition/retexture the shared tips button entity
  void ensure_nav_button();
  void set_nav_right();
  void set_nav_left();
  void restore_hud_button();

  // Page switchers
  void show_page1();
  void show_page2();

public:
  // Set by the caller before push_scene("TipsBookState")
  inline static Engine::ECS::EntityID tips_button_id = 0;
  inline static bool is_open = false;

  void load()   override;
  void init()   override;
  void update() override;
  void draw()   override;
  void free()   override;
  void unload() override;
};



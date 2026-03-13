//-----------------------------------------------------------------------------------
// file:    OptionsState.h
// project: Monkey Stole My GPA
//
// brief:   Options menu state with BGM/SFX volume sliders and screen mode toggle.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class OptionsState : public Engine::Scene
{
public:
  void load();
  void init();
  void update();
  void draw();
  void free();
  void unload();

  // Static entity IDs so callbacks in GameLogicEntry can access the screen mode buttons
  static Engine::ECS::EntityID fullscreen_btn_id;
  static Engine::ECS::EntityID windowed_btn_id;
  static Engine::ECS::EntityID screen_mode_visual_id;

  // Textures accessible from callbacks to swap between fullscreen/windowed display
  static Engine::Graphics::Texture fullscreen_tex;
  static Engine::Graphics::Texture windowed_tex;

private:
  // Background entities (from main menu, drawn underneath)
  Engine::ECS::Entity backsky_entity;
  Engine::ECS::Entity carriage_entity;
  Engine::ECS::Entity front_entity;

  // Sky scrolling animation
  float sky_timer = 0.0f;
  float sky_duration = 7.0f;

  // Options panel
  Engine::ECS::Entity panel_entity;       // "New group.png" - the big pink panel

  // Labels
  Engine::ECS::Entity bgm_label_entity;   // "BGM.png"
  Engine::ECS::Entity sfx_label_entity;   // "SFX.png"
  Engine::ECS::Entity screen_label_entity;// "SCREEN.png"

  // Sliders
  Engine::ECS::Entity bgm_slider_entity;
  Engine::ECS::Entity sfx_slider_entity;

  // Screen mode buttons (left = fullscreen, right = windowed)
  Engine::ECS::Entity fullscreen_entity;
  Engine::ECS::Entity windowed_entity;
  Engine::ECS::Entity screen_mode_visual_entity;  // displays current mode texture

  // Textures for slider levels 1-10
  Engine::Graphics::Texture slider_textures[10];

  // Current slider values (1-10)
  int bgm_level = 10;
  int sfx_level = 10;

  // Helper to update slider texture based on level
  void update_slider_texture(Engine::ECS::Entity entity, int level);

  // Helper to check if mouse is within a rectangular area
  bool is_mouse_in_rect(float x, float y, float width, float height);

  // Get slider level from mouse X position within slider bounds
  int get_slider_level_from_mouse(float slider_x, float slider_width);
};

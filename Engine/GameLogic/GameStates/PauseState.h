//-----------------------------------------------------------------------------------
// file:    PauseState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:    
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class PauseState : public Engine::Scene
{
  // Entity IDs for cleanup
  Engine::ECS::EntityID background_id = 0;
  Engine::ECS::EntityID resume_collider_id = 0;
  Engine::ECS::EntityID restart_collider_id = 0;
  Engine::ECS::EntityID options_collider_id = 0;
  Engine::ECS::EntityID exit_collider_id = 0;

public:
  // Static members for hover callbacks to access
  static Engine::ECS::EntityID menu_sprite_id;
  static int hovered_button; // 0 = none, 1 = resume, 2 = restart, 3 = options, 4 = exit
  static Engine::Graphics::Texture nothing_selected_tex;
  static Engine::Graphics::Texture resume_selected_tex;
  static Engine::Graphics::Texture restart_selected_tex;
  static Engine::Graphics::Texture options_selected_tex;
  static Engine::Graphics::Texture exit_selected_tex;

  /*!
   * @brief Loads resources needed by the pause state.
   */
  void load();

  /*!
   * @brief Initializes the pause state.
   */
  void init();

  /*!
   * @brief Updates the pause state logic.
   */
  void update();

  /*!
   * @brief Renders the pause interface.
   */
  void draw();

  /*!
   * @brief Frees runtime resources used by the pause state.
   */
  void free();

  /*!
   * @brief Unloads resources used by the pause state.
   */
  void unload();
};
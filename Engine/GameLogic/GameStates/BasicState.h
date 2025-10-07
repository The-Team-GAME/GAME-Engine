//-----------------------------------------------------------------------------------
// file:    BasicState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the BasicState which demonstrates basic rendering
//          capabilities including shapes, text, and audio playback. Serves as
//          a test state for fundamental engine features.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h" 
#include "TypeDef.h"
#include "ECS/ECS.h"
#include "Graphics/Renderer.h"

class BasicState : public Engine::GameState<BasicState>
{
private:
  // Entities for rendering
  ECS::Entity rect_top_left;
  ECS::Entity rect_bottom_right;
  ECS::Entity rect_bottom_left;
  ECS::Entity rect_top_right;
  ECS::Entity rect_center;

  ECS::Entity text_hello_1;
  ECS::Entity text_hello_2;
  ECS::Entity text_instruction;

  // Font resources
  Engine::Graphics::Font font_roboto;
  Engine::Graphics::Font font_playfair;

  f32 rotation = 0.f;

public:
  /*!
   * @brief Loads resources needed by the main menu state.
   */
  void load();

  /*!
   * @brief Initializes the main menu state.
   */
  void init();

  /*!
   * @brief Updates the main menu state logic.
   */
  void update();

  /*!
   * @brief Renders the main menu interface.
   */
  void draw();

  /*!
   * @brief Frees runtime resources used by the main menu state.
   */
  void free();

  /*!
   * @brief Unloads resources used by the main menu state.
   */
  void unload();
};
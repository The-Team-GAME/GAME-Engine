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
#include "../../Engine/MonkeyEngine.h"

class BasicState : public Engine::Scene
{
private:
  // Entities for rendering
  Engine::ECS::Entity rect_top_left;
  Engine::ECS::Entity rect_bottom_right;
  Engine::ECS::Entity rect_bottom_left;
  Engine::ECS::Entity rect_top_right;
  Engine::ECS::Entity rect_center;

  Engine::ECS::Entity text_hello_1;
  Engine::ECS::Entity text_hello_2;
  Engine::ECS::Entity text_instruction;

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
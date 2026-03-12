//-----------------------------------------------------------------------------------
// file:    VictoryState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Victory screen state displayed when the player wins.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

class VictoryState : public Engine::Scene
{
  // Entity IDs for cleanup
  Engine::ECS::EntityID background_id = 0;
  Engine::ECS::EntityID victory_screen_id = 0;

public:
  /*!
   * @brief Loads resources needed by the victory state.
   */
  void load();

  /*!
   * @brief Initializes the victory state.
   */
  void init();

  /*!
   * @brief Updates the victory state logic.
   */
  void update();

  /*!
   * @brief Renders the victory interface.
   */
  void draw();

  /*!
   * @brief Frees runtime resources used by the victory state.
   */
  void free();

  /*!
   * @brief Unloads resources used by the victory state.
   */
  void unload();
};

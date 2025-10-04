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

class BasicState : public Engine::GameState<BasicState>
{
private:
  s32 font_id_1 = -1;
  s32 font_id_2 = -1;
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
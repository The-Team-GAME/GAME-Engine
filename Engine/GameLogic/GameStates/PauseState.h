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

#include "../../Engine/MonkeyEngine.h"

class PauseState : public Engine::Scene
{
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
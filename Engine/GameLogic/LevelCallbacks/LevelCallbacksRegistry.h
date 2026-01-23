//-----------------------------------------------------------------------------------
// file:    LevelCallbacksRegistry.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Central registration point for all level callbacks. Call register_all()
//          during game initialization to make all callbacks available to levels.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "GameplayCallbacks.h"
#include "MainMenuCallbacks.h"

namespace LevelCallbacks {

  // Register all level callbacks - call this during game initialization
  inline void register_all() {
    Gameplay::register_callbacks();
    MainMenu::register_callbacks();
  }

} // namespace LevelCallbacks

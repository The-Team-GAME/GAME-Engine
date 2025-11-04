//-----------------------------------------------------------------------------------
// file:    Core.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (50%)
//          Cheng Ming Xian (50%)

// email:   chew.s@digipen.edu, c.mingxian@digipen.edu
//
// brief:   This file defines the Core class which is the main entry point for the
//          game engine. Manages the game loop, subsystem initialization, and provides
//          global access to major engine systems (Graphics, ECS, Audio, etc.).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include <string>

// This will initialize most of the engine subsystems
#include "Precompiled.h"
#include "TypeDef.h"
#include "StateManager.h"

namespace Engine
{
  /* ========================= //
  // Main engine core class    //
  // ========================= */
  class Core
  {
  private:
    bool isRunning;

  public:
    Core(std::string first_scene);
    ~Core();

    void run();

    void stop();
  };
}
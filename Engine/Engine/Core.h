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

    // Game loop state variables
    static f64 accumulator;
    static s32 step_limit;
    static bool do_render;
    static bool physics_step_mode;

  public:
    Core();
    ~Core();

    void run(std::string first_scene);

    void stop();

    // Getters for game loop state
    static bool& is_paused() { return physics_step_mode; }
    static f64 get_accumulator() { return accumulator; }
    static s32 get_step_limit() { return step_limit; }
    static void set_step_limit(s32 limit) { step_limit = limit; }
  };
}
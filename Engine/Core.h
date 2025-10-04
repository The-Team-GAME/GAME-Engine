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

#include "Precompiled.h"
#include "Graphics.h"
#include "ECS/ECS.h"
#include "ObjectFactory.h"
//#include "LevelLoader.h"
#include "Time.h"
#include "ImGuiManager.h"
#include "WindowManager.h"
#include "AudioManager.h"
//#include "ResourceManager.h"
#include "Perf.h"

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
    Core();
    ~Core();

    void run();
  };

  /* ======================================= //
  // Global engine system instances         //
  // ======================================= */
  // extern ECS::World WORLD;
  //extern ObjectFactory FACTORY;
  //extern LevelLoader LEVEL_LOADER;
  //extern Graphics GRAPHICS;
  extern AudioManager AUDIO;
  //extern ResourceManager RESOURCES;
  extern Perf PERFORMANCE;
}
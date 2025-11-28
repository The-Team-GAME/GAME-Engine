//-----------------------------------------------------------------------------------
// file:    GameplayState.h
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

/* EVERY CODE HERE IS TEMPORARY */

#include "../../Engine/MonkeyEngine.h"
 

class GameplayState : public Engine::Scene
{ 
public:
  void load() {
    Engine::Resources::open_level("levels/LevelOne.json");
  };

  void init() {};

  void update() {};

  void draw() {};

  void free() {};

  void unload() {};
};
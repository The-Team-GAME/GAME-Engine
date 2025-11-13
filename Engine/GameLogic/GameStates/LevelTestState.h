//-----------------------------------------------------------------------------------
// file:    LevelTestState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a an empty state, useful for testing
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

class LevelTestState : public Engine::Scene
{
private:
public:
  void load() {


    auto level = Engine::FileIO::deserialize_level_from_json("Assets/levels/LevelTest.json");

    level.instantiate_into_world();

  };

  void init() {};

  void update() {};

  void draw() {
    Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });
  };

  void free() {};

  void unload() {

    // destroy all entities
    Engine::ECS::destroy_all_entities();

  };
};
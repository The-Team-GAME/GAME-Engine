//-----------------------------------------------------------------------------------
// file:    LuaState.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Declares the LuaState scene for the game engine. Integrates Lua scripting
//          for entity creation and behavior logic. Handles resource management, scene
//          lifecycle, and cleanup of LuaState-tagged entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "../../Engine/MonkeyEngine.h"

#include "ShowcaseState.h"

class LuaState : public Engine::Scene
{
private:

public:
  void load() {
    Engine::Resources::set_resource_path("Assets/");

    Engine::LuaManager::call_function("create_test");
  };

  void init() {};

  void update() {
    Engine::LuaManager::call_function("update_movement", static_cast<f32>(Engine::Time::get_fixed_dt()));
    Engine::LuaManager::call_function("chase", static_cast<f32>(Engine::Time::get_fixed_dt()));
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.2f, 0.2f, 0.2f, 1.0f });
  };

  void free() {
  };

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"LuaState">>();
    Engine::LuaManager::call_function("free_resources");
  };
};
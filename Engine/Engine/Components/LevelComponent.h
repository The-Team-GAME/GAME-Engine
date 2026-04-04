//-----------------------------------------------------------------------------------
// file:    LevelComponent.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Component that marks an entity as a Level container.
//          Level entities serve as the root parent for all entities within a loaded level.
//          Stores level metadata including path and lifecycle callbacks.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../ECS/ECS.h"
#include "../ECS/Macros.h"
#include "../FunctionRegistry.h"

namespace Components
{
  struct Level {
    std::string level_path;

    // Lifecycle callbacks (resolved via FunctionRegistry)
    Engine::SerializableFunction on_init;    // Called when level is loaded
    Engine::SerializableFunction on_update;  // Called every frame
    Engine::SerializableFunction on_exit;    // Called when level is unloaded

    Level() = default;
    Level(const std::string& path) : level_path(path) {}

    // Call lifecycle callbacks
    void call_init() const {
      if (!on_init.empty()) {
        auto func = on_init.get<void()>();
        if (func) func();
      }
    }

    void call_update() const {
      if (!on_update.empty()) {
        auto func = on_update.get<void()>();
        if (func) func();
      }
    }

    void call_exit() const {
      if (!on_exit.empty()) {
        auto func = on_exit.get<void()>();
        if (func) func();
      }
    }
  };
}

REGISTER_COMPONENT(Components::Level, level_path, on_init, on_update, on_exit)


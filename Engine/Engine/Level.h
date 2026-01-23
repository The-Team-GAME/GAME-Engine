//-----------------------------------------------------------------------------------
// file:    Level.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Level struct for serializing and deserializing
//          game levels, including entity data and lifecycle callbacks.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "Reflection/ReflectionLibrary/GenericValue.h"
#include "ECS/ECS.h"
#include "FunctionRegistry.h"


namespace Engine {

  // Callback signatures for level lifecycle
  using LevelCallback = void();

  struct Level {
    std::vector<ReflectionLibrary::GenericValue> entities;

    // Lifecycle callbacks (resolved via FunctionRegistry)
    SerializableFunction on_init;    // Called when level is loaded
    SerializableFunction on_update;  // Called every frame
    SerializableFunction on_exit;    // Called when level is unloaded

    static Level from_world();

    void instantiate_into_world(bool clear_world = true);

    // Call lifecycle callbacks
    void call_init() const;
    void call_update() const;
    void call_exit() const;
  };

}
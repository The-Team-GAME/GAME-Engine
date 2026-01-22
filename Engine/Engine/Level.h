//-----------------------------------------------------------------------------------
// file:    Level.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Level struct and LevelManager namespace, providing functionality for serializing and deserializing game levels, including entity and prefab management.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "Reflection/ReflectionLibrary/GenericValue.h"
#include "ECS/ECS.h"


namespace Engine {

  struct Level {
    std::vector<ReflectionLibrary::GenericValue> entities;

    static Level from_world();

    void instantiate_into_world(bool clear_world = true);

  };

  namespace LevelManager {
    std::string get_current_level_path();
    void set_current_level_path(const std::string& level_name); // Only used in SceneManager, when loading scenes instead of levels

    // Destroys all entities that belong to the current level (have LevelMember component)
    // Optionally specify a level path, or leave empty to clear current level
    void clear_level_entities(const std::string& level_path = "");
  }

}
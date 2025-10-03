//-----------------------------------------------------------------------------------
// file:    LevelLoader.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the LevelLoader which manages loading, spawning, and
//          unloading game levels from JSON files. Handles prefab loading, entity
//          instantiation, and level lifecycle management.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <unordered_map>
#include "Level.h"
#include "ECS/World.h"
#include "ObjectFactory.h"

namespace Engine
{
  class LevelLoader
  {
  private:
    ECS::World* world;
    ObjectFactory* factory;
    std::unordered_map<std::string, Level> loaded_levels;
    std::string current_level_id;

  public:
    LevelLoader(ECS::World* world, ObjectFactory* factory);

    /**
     * @brief Loads a level from a JSON file.
     *
     * @param filePath Path to the level JSON file
     * @return std::string The level ID if successful, empty string otherwise
     */
    std::string load_level(const std::string& filePath);

    /**
     * @brief Spawns all entities defined in a loaded level.
     *
     * @param levelId The ID of the level to spawn
     * @return bool True if successful, false otherwise
     */
    bool spawn_level(const std::string& levelId);

    /**
     * @brief Unloads a level, destroying all spawned entities and unloading prefabs.
     *
     * @param levelId The ID of the level to unload
     */
    void unload_level(const std::string& levelId);

    /**
     * @brief Unloads the current active level.
     */
    void unload_current_level();

    /**
     * @brief Gets the current level ID.
     *
     * @return std::string The current level ID
     */
    std::string get_current_level_id() const { return current_level_id; }

    /**
     * @brief Checks if a level is loaded.
     *
     * @param levelId The level ID to check
     * @return bool True if loaded, false otherwise
     */
    bool is_level_loaded(const std::string& levelId) const;

  private:
    Level parse_level(const std::string& filePath);
  };
}
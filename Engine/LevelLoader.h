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
    ECS::World* m_world;
    ObjectFactory* m_factory;
    std::unordered_map<std::string, Level> loadedLevels;
    std::string currentLevelId;

  public:
    LevelLoader(ECS::World* world, ObjectFactory* factory);

    /**
     * @brief Loads a level from a JSON file.
     *
     * @param filePath Path to the level JSON file
     * @return std::string The level ID if successful, empty string otherwise
     */
    std::string loadLevel(const std::string& filePath);

    /**
     * @brief Spawns all entities defined in a loaded level.
     *
     * @param levelId The ID of the level to spawn
     * @return bool True if successful, false otherwise
     */
    bool spawnLevel(const std::string& levelId);

    /**
     * @brief Unloads a level, destroying all spawned entities and unloading prefabs.
     *
     * @param levelId The ID of the level to unload
     */
    void unloadLevel(const std::string& levelId);

    /**
     * @brief Unloads the current active level.
     */
    void unloadCurrentLevel();

    /**
     * @brief Gets the current level ID.
     *
     * @return std::string The current level ID
     */
    std::string getCurrentLevelId() const { return currentLevelId; }

    /**
     * @brief Checks if a level is loaded.
     *
     * @param levelId The level ID to check
     * @return bool True if loaded, false otherwise
     */
    bool isLevelLoaded(const std::string& levelId) const;

  private:
    Level parseLevel(const std::string& filePath);
  };
}
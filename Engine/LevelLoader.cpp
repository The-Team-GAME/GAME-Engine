//-----------------------------------------------------------------------------------
// file:    LevelLoader.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LevelLoader which handles loading levels from
//          JSON files, spawning entities with proper positioning, and managing
//          level lifecycle (load, spawn, unload).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LevelLoader.h"
#include "FileHandler.h"
#include "Logger.h"
#include "Core.h"
#include <iostream>

namespace fh = FileHandler;

namespace Engine
{
  LevelLoader::LevelLoader(ECS::World* world, ObjectFactory* factory)
    : m_world(world), m_factory(factory)
  {
  }

  Level LevelLoader::parseLevel(const std::string& filePath)
  {
    Level level;

    fh::parse_file(filePath);

    // Parse level ID
    if (fh::has_key("levelId"))
    {
      level.levelId = fh::get_string("levelId");
      if (level.levelId == fh::d_string())
      {
        Log::warning("Level ID is empty in " + filePath);
      }
    }
    else
    {
      Log::warning("Level file missing 'levelId' key: " + filePath);
    }

    // Parse object prefab paths from array
    if (fh::is_array("objectPrefabs"))
    {
      s32 prefabCount = fh::get_array_size("objectPrefabs");
      for (s32 i = 0; i < prefabCount; ++i)
      {
        std::string prefabPath = fh::get_string_from_array("objectPrefabs", i);
        if (prefabPath != fh::d_string())
        {
          level.objectPrefabPaths.push_back(prefabPath);
        }
      }
    }

    // Parse entities from array
    if (fh::is_array("entities"))
    {
      s32 entityCount = fh::get_array_size("entities");
      for (s32 i = 0; i < entityCount; ++i)
      {
        EntityData entity;

        entity.prefabId = fh::get_string_from_array_object("entities", i, "id");
        entity.x = fh::get_f32_from_array_object("entities", i, "x");
        entity.y = fh::get_f32_from_array_object("entities", i, "y");

        if (entity.prefabId != fh::d_string())
        {
          level.entities.push_back(entity);
        }
      }
    }

    fh::clear_json();

    Log::info("Successfully parsed level: " + level.levelId);
    Log::info("  Prefabs: " + std::to_string(level.objectPrefabPaths.size()));
    Log::info("  Entities: " + std::to_string(level.entities.size()));

    return level;
  }

  std::string LevelLoader::loadLevel(const std::string& filePath)
  {
    Level level = parseLevel(filePath);

    if (level.levelId.empty())
    {
      Log::warning("Failed to load level from: " + filePath);
      return "";
    }

    // Check if level already loaded
    if (isLevelLoaded(level.levelId))
    {
      Log::warning("Level already loaded: " + level.levelId);
      return level.levelId;
    }

    // Load all object prefabs
    for (const auto& prefabPath : level.objectPrefabPaths)
    {
      s32 prefabId = m_factory->registerObject(prefabPath);
      if (prefabId != -1)
      {
        level.loadedPrefabIds.push_back(prefabId);
        Log::info("Loaded prefab: " + prefabPath + " (ID: " + std::to_string(prefabId) + ")");
      }
      else
      {
        Log::warning("Failed to load prefab: " + prefabPath);
      }
    }

    // Store the level
    loadedLevels[level.levelId] = level;

    Log::info("Level loaded successfully: " + level.levelId);
    return level.levelId;
  }

  bool LevelLoader::spawnLevel(const std::string& levelId)
  {
    auto it = loadedLevels.find(levelId);
    if (it == loadedLevels.end())
    {
      Log::warning("Cannot spawn level - not loaded: " + levelId);
      return false;
    }

    Level& level = it->second;

    // Clear any previously spawned entities
    for (auto entityId : level.spawnedEntities)
    {
      m_world->destroy_entity(entityId);
    }
    level.spawnedEntities.clear();

    // Spawn all entities
    for (const auto& entityData : level.entities)
    {
      // Find the prefab ID by matching the prefab's object ID
      s32 prefabIdToUse = -1;
      for (int prefabId : level.loadedPrefabIds)
      {
        // Create a temporary entity to check its ObjectId
        auto tempEntity = m_factory->createObject(prefabId);
        auto* objectId = tempEntity.get_component<Components::ObjectId>();

        if (objectId && *objectId == entityData.prefabId)
        {
          tempEntity.destroy();
          prefabIdToUse = prefabId;
          break;
        }
        tempEntity.destroy();
      }

      if (prefabIdToUse == -1)
      {
        Log::warning("Prefab not found for entity: " + entityData.prefabId);
        continue;
      }

      // Create entity from prefab
      auto entity = m_factory->createObject(prefabIdToUse);

      // Set position
      entity.add_component<Components::Position>(entityData.x, entityData.y);

      // Track spawned entity
      level.spawnedEntities.push_back(entity.getId());
    }

    currentLevelId = levelId;
    Log::info("Spawned level: " + levelId + " with " +
      std::to_string(level.spawnedEntities.size()) + " entities");

    return true;
  }

  void LevelLoader::unloadLevel(const std::string& levelId)
  {
    auto it = loadedLevels.find(levelId);
    if (it == loadedLevels.end())
    {
      Log::warning("Cannot unload level - not loaded: " + levelId);
      return;
    }

    Level& level = it->second;

    // Destroy all spawned entities
    for (auto entityId : level.spawnedEntities)
    {
      m_world->destroy_entity(entityId);
    }
    level.spawnedEntities.clear();

    // Unload all prefabs
    for (int prefabId : level.loadedPrefabIds)
    {
      m_factory->unregisterObject(prefabId);
    }
    level.loadedPrefabIds.clear();

    // Remove level from loaded levels
    loadedLevels.erase(it);

    if (currentLevelId == levelId)
    {
      currentLevelId.clear();
    }

    Log::info("Unloaded level: " + levelId);
  }

  void LevelLoader::unloadCurrentLevel()
  {
    if (!currentLevelId.empty())
    {
      unloadLevel(currentLevelId);
    }
  }

  bool LevelLoader::isLevelLoaded(const std::string& levelId) const
  {
    return loadedLevels.find(levelId) != loadedLevels.end();
  }
}
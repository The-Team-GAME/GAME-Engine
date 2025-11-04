//-----------------------------------------------------------------------------------
// file:    LevelFileIO.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements serialization and deserialization functions for
//          game levels. Provides low-level JSON conversion utilities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LevelFileIO.h"
#include "../ECS/ECS.h"
#include "../Logger.h"
#include "Helpers.h"
#include "../Reflection/ReflectionLibrary.h"
#include <filesystem>

namespace Engine {
  namespace FileIO {

    std::string serialize_level_to_json(const Engine::LevelManager::Level& level) {
      // Create root object
      ReflectionLibrary::Object root_obj;

      // Add level name
      root_obj["name"] = ReflectionLibrary::GenericValue(level.name);

      // Add entities array
      ReflectionLibrary::Array entities_array;

      for (const auto& entity : level.entities) {
        if (entity.is_valid()) {
          ReflectionLibrary::GenericValue entity_value = Engine::ECS::entity_to_generic_value(entity);
          entities_array.push_back(entity_value);
        }
      }

      root_obj["entities"] = ReflectionLibrary::GenericValue(std::move(entities_array));

      // Convert to JSON string
      ReflectionLibrary::GenericValue root(std::move(root_obj));
      return ReflectionLibrary::generic_value_to_json(root, true);
    }

    Engine::LevelManager::Level deserialize_level_from_json(const std::string& json_str) {
      Engine::LevelManager::Level level;

      std::optional<ReflectionLibrary::GenericValue> root = ReflectionLibrary::generic_value_from_json(json_str);
      if (!root) {
        Log::error("Failed to parse level JSON");
        return level;
      }

      // Get level name
      if (root->has("name") && (*root)["name"].isString()) {
        level.name = (*root)["name"].getString();
      }

      // Get entities array
      if (!root->has("entities") || !(*root)["entities"].isArray()) {
        Log::error("Level JSON missing 'entities' array");
        return level;
      }

      const auto& entities_array = (*root)["entities"].getArray();
      level.entities.reserve(entities_array.size());

      for (const auto& entity_value : entities_array) {
        Engine::ECS::Entity entity = Engine::ECS::create_entity_from_generic_value(entity_value);
        if (entity.is_valid()) {
          level.entities.push_back(entity);
        }
      }

      return level;
    }

    bool serialize_level_to_json_file(const std::string& filepath, const Engine::LevelManager::Level& level) {
      std::string json_content = serialize_level_to_json(level);

      if (write_string_to_file(filepath, json_content)) {
        Log::info("Level saved to: " + filepath);
        return true;
      }
      else {
        Log::error("Failed to save level to: " + filepath);
        return false;
      }
    }

    Engine::LevelManager::Level deserialize_level_from_json_file(const std::string& filepath) {
      Engine::LevelManager::Level level;

      std::optional<std::string> json_content = read_string_from_file(filepath);
      if (!json_content) {
        Log::error("Failed to read level file: " + filepath);
        return level;
      }

      level = deserialize_level_from_json(json_content.value());
      Log::info("Level loaded from: " + filepath);
      return level;
    }

  } // namespace FileIO
} // namespace Engine

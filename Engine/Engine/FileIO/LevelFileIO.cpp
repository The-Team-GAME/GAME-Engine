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
    std::string serialize_level_to_json(const Engine::Level& level)
    {

      ReflectionLibrary::GenericValue level_value = ReflectionLibrary::Object();

      level_value["dataType"] = "Level";
      level_value["data"]["entities"] = ReflectionLibrary::Array();

      for (const auto& entity_value : level.entities) {
        level_value["data"]["entities"].push_back(entity_value);
      }

      // Serialize callbacks (only if set)
      if (!level.on_init.empty()) {
        level_value["data"]["on_init"] = level.on_init.name;
      }
      if (!level.on_update.empty()) {
        level_value["data"]["on_update"] = level.on_update.name;
      }
      if (!level.on_exit.empty()) {
        level_value["data"]["on_exit"] = level.on_exit.name;
      }

      return ReflectionLibrary::generic_value_to_json(level_value, true);
    }

    Engine::Level deserialize_level_from_json(const std::string& json_str)
    {
      std::optional<ReflectionLibrary::GenericValue> level_value_opt = ReflectionLibrary::generic_value_from_json(json_str);
      if (!level_value_opt) {
        Log::warning("Failed to parse level JSON");
        return Engine::Level();
      }
      ReflectionLibrary::GenericValue level_value = level_value_opt.value();

      Engine::Level level;

      const auto& entities_array = level_value["data"]["entities"].getArray();
      for (const auto& entity_value : entities_array) {
        level.entities.push_back(entity_value);
      }

      // Deserialize callbacks (if present)
      if (level_value["data"].has("on_init") && level_value["data"]["on_init"].isString()) {
        level.on_init = level_value["data"]["on_init"].getString();
      }
      if (level_value["data"].has("on_update") && level_value["data"]["on_update"].isString()) {
        level.on_update = level_value["data"]["on_update"].getString();
      }
      if (level_value["data"].has("on_exit") && level_value["data"]["on_exit"].isString()) {
        level.on_exit = level_value["data"]["on_exit"].getString();
      }

      return level;
    }

    bool serialize_level_to_json_file(const std::string& filepath, const Engine::Level& level)
    {
      std::string json_str = serialize_level_to_json(level);
      return Engine::FileIO::write_string_to_file(filepath, json_str);
    }

    Engine::Level deserialize_level_from_json_file(const std::string& filepath)
    {
      auto json_str = Engine::FileIO::read_string_from_file(filepath);

      if (!json_str.has_value()) {
        Log::warning("Failed to read level file: {}", filepath);
        return Engine::Level();
      }

      return deserialize_level_from_json(json_str.value());
    }



  } // namespace FileIO
} // namespace Engine

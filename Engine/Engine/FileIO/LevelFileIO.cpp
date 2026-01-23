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
    std::string serialize_level_to_json(const Engine::EntityList& entities)
    {

      ReflectionLibrary::GenericValue level_value = ReflectionLibrary::Object();

      level_value["dataType"] = "Level";
      level_value["data"]["entities"] = ReflectionLibrary::Array();

      for (const auto& entity_value : entities) {
        level_value["data"]["entities"].push_back(entity_value);
      }

      return ReflectionLibrary::generic_value_to_json(level_value, true);
    }

    Engine::EntityList deserialize_level_from_json(const std::string& json_str)
    {
      std::optional<ReflectionLibrary::GenericValue> level_value_opt = ReflectionLibrary::generic_value_from_json(json_str);
      if (!level_value_opt) {
        Log::warning("Failed to parse level JSON");
        return Engine::EntityList();
      }
      ReflectionLibrary::GenericValue level_value = level_value_opt.value();

      Engine::EntityList entities;

      const auto& entities_array = level_value["data"]["entities"].getArray();
      for (const auto& entity_value : entities_array) {
        entities.push_back(entity_value);
      }

      return entities;
    }

    bool serialize_level_to_json_file(const std::string& filepath, const Engine::EntityList& entities)
    {
      std::string json_str = serialize_level_to_json(entities);
      return Engine::FileIO::write_string_to_file(filepath, json_str);
    }

    Engine::EntityList deserialize_level_from_json_file(const std::string& filepath)
    {
      auto json_str = Engine::FileIO::read_string_from_file(filepath);

      if (!json_str.has_value()) {
        Log::warning("Failed to read level file: {}", filepath);
        return Engine::EntityList();
      }

      return deserialize_level_from_json(json_str.value());
    }



  } // namespace FileIO
} // namespace Engine

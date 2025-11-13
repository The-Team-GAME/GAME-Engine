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

      ReflectionLibrary::GenericValue level_value = ReflectionLibrary::GenericValue().getObject();

      level_value.getObject().at("name") = ReflectionLibrary::GenericValue(level.name);
      ReflectionLibrary::GenericValue entities_array = ReflectionLibrary::GenericValue().getArray();
      for (const auto& entity_value : level.entities) {
        entities_array.getArray().push_back(entity_value);
      }
      level_value.getObject().at("entities") = entities_array;

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
      level.name = level_value.getObject().at("name").getString();

      const auto& entities_array = level_value.getObject().at("entities").getArray();
      for (const auto& entity_value : entities_array) {
        level.entities.push_back(entity_value);
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
        Log::warning("Failed to read level file: " + filepath);
        return Engine::Level();
      }

      return deserialize_level_from_json(json_str.value());
    }



  } // namespace FileIO
} // namespace Engine

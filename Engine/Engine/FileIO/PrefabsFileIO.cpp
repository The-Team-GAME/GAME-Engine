//-----------------------------------------------------------------------------------
// file:    PrefabsFileIO.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   File I/O implementation for prefab serialization and deserialization.
//          Provides JSON-based save/load functionality for ECS prefabs using
//          the reflection library. Handles file operations, error checking,
//          and integration with the prefab system for game asset management.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../ECS/ECS.h"
#include "PrefabsFileIO.h"
#include <fstream>
#include "../Logger.h"
#include "Helpers.h"

namespace Engine {
  namespace FileIO {
    std::string serialize_prefab_to_json(const Engine::ECS::Prefab& prefab) {
      if (!prefab.is_valid()) {
        return "{}";
      }
      ReflectionLibrary::GenericValue generic_value = ReflectionLibrary::Object();

      generic_value["dataType"] = "Prefab";
      generic_value["data"] = Engine::ECS::prefab_to_generic_value(prefab);
      
      return ReflectionLibrary::generic_value_to_json(generic_value, true);
    }

    Engine::ECS::Prefab deserialize_prefab_from_generic_value(const ReflectionLibrary::GenericValue& generic_value) {
      if (!generic_value.isObject() || !generic_value.getObject().contains("dataType") ||
          generic_value["dataType"].getString() != "Prefab" ||
          !generic_value.getObject().contains("data")) {

            Log::warning("Invalid prefab JSON format: missing or incorrect dataType or data fields");
            return Engine::ECS::Prefab(); // Return empty prefab handle
      }

      return Engine::ECS::prefab_from_generic_value(generic_value["data"]);
    }

    Engine::ECS::Prefab deserialize_prefab_from_json(const std::string& json_str) {
      std::optional<ReflectionLibrary::GenericValue> generic_value_opt = ReflectionLibrary::generic_value_from_json(json_str);
      if (!generic_value_opt.has_value()) {
        return Engine::ECS::Prefab(); // Return empty prefab handle
      }

      return deserialize_prefab_from_generic_value(generic_value_opt.value());
    }

    bool serialize_prefab_to_json_file(const std::string& filepath, const Engine::ECS::Prefab& prefab)
    {
      // Serialize prefab to JSON
      std::string json_content = Engine::FileIO::serialize_prefab_to_json(prefab);

      Engine::FileIO::write_string_to_file(filepath, json_content);
      
      Log::info("Prefab saved: {}", filepath);

      return true;
    }


    Engine::ECS::Prefab deserialize_prefab_from_json_file(const std::string& filepath)
    {
       std::optional<std::string> json_content = Engine::FileIO::read_string_from_file(filepath);

      if (!json_content.has_value())
      {
        Log::warning("Failed to read prefab file: {}", filepath);
        return Engine::ECS::Prefab(); // Return empty prefab handle
      }

      return deserialize_prefab_from_json(json_content.value());
    }

  }
} 
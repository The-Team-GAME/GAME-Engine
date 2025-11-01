#include "../ECS/ECS.h"
#include "PrefabsFileIO.h"
#include <fstream>
#include "../Logger.h"

namespace Engine {
  namespace FileIO {
    std::string serialize_prefab_to_json(const Engine::ECS::Prefab& prefab) {
      if (!prefab.is_valid()) {
        return "{}";
      }
      ReflectionLibrary::GenericValue generic_value = Engine::ECS::prefab_to_generic_value(prefab);
      return ReflectionLibrary::generic_value_to_json(generic_value, true);
    }

    Engine::ECS::Prefab deserialize_prefab_from_json(const std::string& json_str) {
      std::optional<ReflectionLibrary::GenericValue> generic_value = ReflectionLibrary::generic_value_from_json(json_str);
      if (!generic_value) {
        return Engine::ECS::Prefab(); // Return empty prefab handle
      }
      return Engine::ECS::prefab_from_generic_value(generic_value.value());
    }

    bool serialize_prefab_to_json_file(const std::string& filepath, const Engine::ECS::Prefab& prefab)
    {
      // Serialize prefab to JSON
      std::string json_content = Engine::FileIO::serialize_prefab_to_json(prefab);

      if (json_content.empty() || json_content == "{}")
      {
        Log::warning("Failed to serialize prefab: " + filepath);
        return false;
      }

      // TODO: this needs to create the folder if it doesn't exist
      std::ofstream file(filepath);
      if (!file.is_open())
      {
        Log::warning("Failed to open prefab file for writing: " + filepath);
        return false;
      }

      file << json_content;
      file.close();

      
      Log::info("Prefab saved: " + filepath);

      return true;
    }


    Engine::ECS::Prefab deserialize_prefab_from_json_file(const std::string& filepath)
    {
      // Load prefab from file
      std::ifstream file(filepath);
      if (!file.is_open())
      {
        Log::warning("Failed to open prefab file: " + filepath);
        return Engine::ECS::Prefab(); // Return empty prefab handle
      }

      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string json_content = buffer.str();
      file.close();

      return deserialize_prefab_from_json(json_content);
    }

  }
} 
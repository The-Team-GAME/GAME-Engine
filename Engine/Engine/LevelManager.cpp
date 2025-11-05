//-----------------------------------------------------------------------------------
// file:    LevelManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LevelManager for managing game levels.
//          Handles saving/loading levels to/from the ECS world with layer filtering.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LevelManager.h"
#include "ECS/ECS.h"
#include "Logger.h"
#include "FileIO/LevelFileIO.h"

namespace Engine {
  namespace LevelManager {

    std::vector<Engine::ECS::Entity> get_all_entities_in_world(
      const Engine::ECS::LayerSignature& layer_filter
    ) {
      std::vector<Engine::ECS::Entity> entities;

      // Get all entities from the world
      Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
        // If no filter specified, add all entities
        if (layer_filter.none()) {
          entities.push_back(entity);
        }
        else {
          // Check if entity is on any of the filtered layers
          Engine::ECS::LayerSignature entity_layers = entity.get_layers();
          if ((entity_layers & layer_filter).any()) {
            entities.push_back(entity);
          }
        }
        });

      return entities;
    }

    bool save_current_world_as_level(const std::string& filepath,
      const std::string& level_name,
      const Engine::ECS::LayerSignature& layer_filter) {
      Level level(level_name);
      level.entities = get_all_entities_in_world(layer_filter);

      Log::info("Saving " + std::to_string(level.entities.size()) + " entities to level: " + level_name);
      return Engine::FileIO::serialize_level_to_json_file(filepath, level);
    }

    bool load_level_into_world(const std::string& filepath, bool clear_existing) {
      // Clear existing entities if requested
      if (clear_existing) {
        Log::info("Clearing existing entities before loading level");
        std::vector<Engine::ECS::Entity> existing_entities = get_all_entities_in_world();
        for (auto& entity : existing_entities) {
          Engine::ECS::destroy_entity(entity);
        }
      }

      // Load the level
      Level level = Engine::FileIO::deserialize_level_from_json_file(filepath);

      if (level.entities.empty()) {
        Log::warning("Loaded level is empty or failed to load: " + filepath);
        return false;
      }

      Log::info("Loaded level '" + level.name + "' with " + std::to_string(level.entities.size()) + " entities");
      return true;
    }

    bool save_current_world_as_level_with_component_filter(
      const std::string& filepath,
      const std::string& level_name,
      const std::vector<std::string>& component_names
    ) {
      Level level(level_name);

      // Get all entities from the world
      auto all_entities = Engine::ECS::get_all_entities();

      // Filter entities that have at least one of the specified components
      for (auto& entity : all_entities) {
        bool matches = false;

        // Check if entity has any of the specified components
        entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
          for (const auto& comp_name : component_names) {
            if (type_name == comp_name) {
              matches = true;
              return; // Exit the lambda early
            }
          }
          });

        if (matches) {
          level.entities.push_back(entity);
        }
      }

      Log::info("Saving " + std::to_string(level.entities.size()) + " filtered entities to level: " + level_name);
      return Engine::FileIO::serialize_level_to_json_file(filepath, level);
    }

  } // namespace LevelManager
} // namespace Engine

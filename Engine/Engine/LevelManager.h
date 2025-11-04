//-----------------------------------------------------------------------------------
// file:    LevelManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares the LevelManager namespace for managing game levels.
//          Provides functions for saving and loading complete game scenes with
//          entities and their components, including layer-based filtering.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "ECS/ECS.h"

namespace Engine {
  namespace LevelManager {

    /**
     * @struct Level
     * @brief Represents a complete game level with all entities and metadata
     */
    struct Level {
      std::string name;
      std::vector<Engine::ECS::Entity> entities;

      Level() = default;
      Level(const std::string& level_name) : name(level_name) {}
    };

    /**
     * @brief Saves the current ECS world as a level
     * @param filepath Path to save the level file
     * @param level_name Name of the level
     * @param layer_filter Optional layer filter - only save entities on these layers
     * @return true if successful, false otherwise
     */
    bool save_current_world_as_level(const std::string& filepath,
      const std::string& level_name = "Unnamed Level",
      const Engine::ECS::LayerSignature& layer_filter = Engine::ECS::LayerSignature());

    /**
     * @brief Loads a level into the current ECS world
     * @param filepath Path to the level file
     * @param clear_existing If true, destroys all existing entities before loading
     * @return true if successful, false otherwise
     */
    bool load_level_into_world(const std::string& filepath, bool clear_existing = true);

    /**
     * @brief Gets all entities currently in the world on specified layers
     * @param layer_filter Optional layer filter - only get entities on these layers
     * @return Vector of all entities matching the filter
     */
    std::vector<Engine::ECS::Entity> get_all_entities_in_world(
      const Engine::ECS::LayerSignature& layer_filter = Engine::ECS::LayerSignature()
    );

    /**
     * @brief Saves the current ECS world as a level with component filtering
     * @param filepath Path to save the level file
     * @param level_name Name of the level
     * @param component_names Set of component names to filter by (saves entities with ANY of these)
     * @return true if successful, false otherwise
     */
    bool save_current_world_as_level_with_component_filter(
      const std::string& filepath,
      const std::string& level_name,
      const std::vector<std::string>& component_names
    );

  } // namespace LevelManager
} // namespace Engine

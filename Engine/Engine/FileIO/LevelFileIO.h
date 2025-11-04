//-----------------------------------------------------------------------------------
// file:    LevelFileIO.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares functions for serializing and deserializing game levels
//          to/from JSON format. Enables saving and loading of complete game scenes
//          with all entities, their components, positions, and metadata.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include "../ECS/ECS.h"

namespace Engine {
  namespace FileIO {

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
     * @brief Serializes a level to JSON string
     * @param level The level to serialize
     * @return JSON string representation of the level
     */
    std::string serialize_level_to_json(const Level& level);

    /**
     * @brief Deserializes a level from JSON string
     * @param json_str The JSON string to deserialize
     * @return The deserialized level
     */
    Level deserialize_level_from_json(const std::string& json_str);

    /**
     * @brief Saves a level to a JSON file
     * @param filepath Path to save the level file
     * @param level The level to save
     * @return true if successful, false otherwise
     */
    bool serialize_level_to_json_file(const std::string& filepath, const Level& level);

    /**
     * @brief Loads a level from a JSON file
     * @param filepath Path to the level file
     * @return The loaded level
     */
    Level deserialize_level_from_json_file(const std::string& filepath);

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

  } // namespace FileIO
} // namespace Engine

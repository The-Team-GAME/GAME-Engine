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
//          to/from JSON format. Provides low-level serialization utilities for the
//          LevelManager to use when saving/loading levels.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../LevelManager.h"

namespace Engine {
  namespace FileIO {

    /**
     * @brief Serializes a level to JSON string
     * @param level The level to serialize
     * @return JSON string representation of the level
     */
    std::string serialize_level_to_json(const Engine::LevelManager::Level& level);

    /**
     * @brief Deserializes a level from JSON string
     * @param json_str The JSON string to deserialize
     * @return The deserialized level
     */
    Engine::LevelManager::Level deserialize_level_from_json(const std::string& json_str);

    /**
     * @brief Saves a level to a JSON file
     * @param filepath Path to save the level file
     * @param level The level to save
     * @return true if successful, false otherwise
     */
    bool serialize_level_to_json_file(const std::string& filepath, const Engine::LevelManager::Level& level);

    /**
     * @brief Loads a level from a JSON file
     * @param filepath Path to the level file
     * @return The loaded level
     */
    Engine::LevelManager::Level deserialize_level_from_json_file(const std::string& filepath);

  } // namespace FileIO
} // namespace Engine

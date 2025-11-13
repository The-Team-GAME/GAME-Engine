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
#include "../Level.h"

namespace Engine {
  namespace FileIO {

    /*=================================================================================
    Function Brief:
    Serializes a Level object to a JSON string.

    Parameters:
    level - The Level object to serialize.

    Returns:
    JSON string containing the serialized level data.

    Where to call it:
    When saving levels to files or transferring level data.
    ==================================================================================*/
    std::string serialize_level_to_json(const Engine::Level& level);

    /*=================================================================================
    Function Brief:
    Deserializes a Level object from a JSON string.

    Parameters:
    json_str - The JSON string containing the serialized level data.

    Returns:
    A Level object reconstructed from the JSON data.

    Where to call it:
    When loading levels from files or receiving level data.
    ==================================================================================*/
    Engine::Level deserialize_level_from_json(const std::string& json_str);


    bool serialize_level_to_json_file(const std::string& filepath, const Engine::Level& level);
    Engine::Level deserialize_level_from_json_file(const std::string& filepath);


  } // namespace FileIO
} // namespace Engine

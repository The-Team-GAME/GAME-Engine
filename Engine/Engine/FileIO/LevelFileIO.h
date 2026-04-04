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
#include <vector>
#include "../Reflection/ReflectionLibrary/GenericValue.h"

namespace Engine {
  // Forward declaration of EntityList type
  using EntityList = std::vector<ReflectionLibrary::GenericValue>;

  namespace FileIO {

    /*=================================================================================
    Function Brief:
    Serializes an EntityList to a JSON string.

    Parameters:
    entities - The entity list to serialize.

    Returns:
    JSON string containing the serialized level data.

    Where to call it:
    When saving levels to files or transferring level data.
    ==================================================================================*/
    std::string serialize_level_to_json(const Engine::EntityList& entities);

    /*=================================================================================
    Function Brief:
    Deserializes an EntityList from a JSON string.

    Parameters:
    json_str - The JSON string containing the serialized level data.

    Returns:
    An EntityList reconstructed from the JSON data.

    Where to call it:
    When loading levels from files or receiving level data.
    ==================================================================================*/
    Engine::EntityList deserialize_level_from_json(const std::string& json_str);


    // Deserializes an EntityList from an already-parsed GenericValue (the root object with dataType/data).
    Engine::EntityList deserialize_level_from_generic_value(const ReflectionLibrary::GenericValue& level_value);

    bool serialize_level_to_json_file(const std::string& filepath, const Engine::EntityList& entities);
    Engine::EntityList deserialize_level_from_json_file(const std::string& filepath);


  } // namespace FileIO
} // namespace Engine


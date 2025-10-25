//-----------------------------------------------------------------------------------
// file:    PrefabsFileIO.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file declares functions for serializing and deserializing ECS
//          prefabs to/from JSON format. Enables saving and loading of entity
//          templates with their component configurations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include "../ECS/ECS.h"

namespace Engine {
  namespace FileIO {
    /*=================================================================================
    Function Brief:
    Serializes a prefab to a JSON string by creating a temporary entity,
    applying the prefab, and serializing its components.

    Parameters:
    prefab - The prefab to serialize.

    Returns:
    JSON string containing the serialized prefab data, or "{}" if prefab is invalid.

    Where to call it:
    When saving prefabs to files or transferring prefab data.
    ==================================================================================*/
    std::string serialize_prefab_to_json(const Engine::ECS::Prefab& prefab);

    /*=================================================================================
    Function Brief:
    Deserializes a prefab from a JSON string, creating a prefab that will apply
    the deserialized components to entities.

    Parameters:
    json_str - The JSON string containing the serialized prefab data.

    Returns:
    A prefab that can be applied to entities, or nullptr if deserialization fails.

    Where to call it:
    When loading prefabs from files or receiving prefab data.
    ==================================================================================*/
    Engine::ECS::Prefab deserialize_prefab_from_json(const std::string& json_str);
  }
}
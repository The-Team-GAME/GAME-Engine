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

    Where to call it:
    When loading prefabs from files or receiving prefab data.
    ==================================================================================*/
    Engine::ECS::Prefab deserialize_prefab_from_json(const std::string& json_str);
  }
}
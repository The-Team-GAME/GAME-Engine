//-----------------------------------------------------------------------------------
// file:    Prefab.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Prefab system for the ECS. Prefabs are reusable
//          entity templates that store component configurations. They can be
//          instantiated multiple times to create entities with the same setup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "Entity.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <rapidjson/document.h>

namespace Engine {
  namespace ECS {
    /* ====================================== //
    // Prefab - Reusable entity template      //
    // ====================================== */
    struct Prefab {
      std::function<void(Entity&)> setup_function;

      Prefab() = default;
      Prefab(std::function<void(Entity&)> setup);
    };

    /* ====================================== //
    // Prefab functions                       //
    // ====================================== */
    Prefab create_prefab(std::function<void(Entity&)> setup);
    Entity instantiate_prefab(const Prefab& prefab);
    std::string serialize_prefab_to_json(const Prefab& prefab);
    Prefab deserialize_prefab_from_json(const std::string& json_str);

  } // namespace ECS
} // namespace Engine
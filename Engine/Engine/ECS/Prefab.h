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

#include "Entity.h"
#include <memory>
#include <string>
#include <functional>
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {
    /* ====================================== //
    // PrefabData - Reusable entity template  //
    // ====================================== */
    struct PrefabData {
      std::function<Entity(void)> setup_function;

      PrefabData() = default;

      /*=================================================================================
      Function Brief:
      Constructs a PrefabData with the provided setup function.

      Parameters:
      setup - A function that configures an entity with components.

      Where to call it:
      When creating prefab data structures.
      ==================================================================================*/
      PrefabData(std::function<Entity(void)> setup);
    };

    using Prefab = std::shared_ptr<PrefabData>;

    /* ====================================== //
    // Prefab functions                       //
    // ====================================== */

    /*=================================================================================
    Function Brief:
    Creates a new prefab with the specified setup function. The setup function
    will be called on each entity instantiated from this prefab.

    Parameters:
    setup - A function that configures an entity with components.

    Where to call it:
    When defining reusable entity templates.
    ==================================================================================*/
    Prefab create_prefab(std::function<Entity(void)> setup);

    /*=================================================================================
    Function Brief:
    Creates a new entity and applies the prefab's setup function to it.

    Parameters:
    prefab - The prefab to instantiate.

    Where to call it:
    When spawning entities based on a prefab template.
    ==================================================================================*/
    Entity instantiate_prefab(const Prefab& prefab);

    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab);

    Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value);
  } // namespace ECS
} // namespace Engine
//-----------------------------------------------------------------------------------
// file:    Prefab.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Prefab system for creating reusable entity
//          templates. Provides functions for creating and instantiating prefabs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Prefab.h"
#include "World.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {

    PrefabData::PrefabData(std::function<Entity(void)> setup)
      : setup_function(std::move(setup)) {
    }

    Prefab create_prefab(std::function<Entity(void)> setup) {
      return std::make_shared<PrefabData>(std::move(setup));
    }

    Entity instantiate_prefab(const Prefab& prefab) {
      if (prefab && prefab->setup_function) {
        return prefab->setup_function();
      }
      return Entity(INVALID_ENTITY);
    }

    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab)
    {
      if (!prefab || !prefab->setup_function) {
        return ReflectionLibrary::GenericValue();
      }

      // Create a temporary entity and apply the prefab setup
      Entity temp_entity = prefab->setup_function();

      // Serialize the temporary entity to GenericValue
      ReflectionLibrary::GenericValue value = entity_to_generic_value(temp_entity);

      // Clean up the temporary entity after serialization
      temp_entity.destroy();

      return value;
    }

    Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value)
    {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return nullptr;
      }

      return create_prefab([value]() {
        return create_entity_from_generic_value(value);
        });
    }

  } // namespace ECS
} // namespace Engine
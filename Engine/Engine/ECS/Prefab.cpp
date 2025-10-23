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

    PrefabData::PrefabData(std::function<void(Entity&)> setup)
      : setup_function(std::move(setup)) {
    }

    Prefab create_prefab(std::function<void(Entity&)> setup) {
      return std::make_shared<PrefabData>(std::move(setup));
    }

    Entity instantiate_prefab(const Prefab& prefab) {
      Entity entity = create_entity();
      if (prefab && prefab->setup_function) {
        prefab->setup_function(entity);
      }
      return entity;
    }

    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab)
    {
      if (!prefab || !prefab->setup_function) {
        return ReflectionLibrary::GenericValue();
      }

      // Create a temporary entity and apply the prefab setup
      Entity temp_entity = create_entity();
      prefab->setup_function(temp_entity);

      return temp_entity.to_generic_value();
    }

    Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value)
    {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return nullptr;
      }

      auto setup_function = [value](Entity& entity) {
        Entity temp_entity(value);
        entity = temp_entity;
        };

      return std::make_shared<PrefabData>(setup_function);
    }

    std::string serialize_prefab_to_json(const Prefab& prefab) {
      if (!prefab || !prefab->setup_function) {
        return "{}";
      }
      ReflectionLibrary::GenericValue generic_value = prefab_to_generic_value(prefab);
      return ReflectionLibrary::generic_value_to_json(generic_value, true);
    }

    Prefab deserialize_prefab_from_json(const std::string& json_str) {
      std::optional<ReflectionLibrary::GenericValue> generic_value = ReflectionLibrary::generic_value_from_json(json_str);
      if (!generic_value) {
        return nullptr;
      }
      return prefab_from_generic_value(generic_value.value());
    }
  } // namespace ECS
} // namespace Engine
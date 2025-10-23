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

    ReflectionLibrary::GenericValue to_generic_value(const Prefab& prefab)
    {
      if (!prefab || !prefab->setup_function) {
        return ReflectionLibrary::GenericValue();
      }

      // Create a temporary entity and apply the prefab setup
      Entity temp_entity = create_entity();
      prefab->setup_function(temp_entity);


      EntityID entity_id = temp_entity.get_id();
      const auto& signature = Internal::entity_signatures[entity_id];

      ReflectionLibrary::GenericValue result;
      result["id"] = entity_id;

      // Serialize each component's data
      for (const auto& [type_id, component_array] : Internal::component_arrays) {
        if (signature.test(type_id) && component_array->is_reflectable()) {
          ReflectionLibrary::GenericValue component_data = component_array->to_generic_value(entity_id);
          ReflectionLibrary::GenericValue component_entry;
          component_entry["type"] = component_array->get_type_name();
          component_entry["data"] = component_data;
          result["components"].push_back(component_entry);
        }
      }

      return result;
    }

    Prefab from_generic_value(const ReflectionLibrary::GenericValue& value)
    {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return nullptr;
      }

      // Create a lambda that will setup the entity with the deserialized components
      auto setup_function = [value](Entity& entity) {
        const auto& components_array = value["components"].getArray();

        // Iterate through each component in the JSON
        for (const auto& component_entry : components_array) {
          if (!component_entry.isObject() || !component_entry.has("type") || !component_entry.has("data")) {
            continue;
          }

          std::string type_name = component_entry["type"].getString();
          const auto& component_data = component_entry["data"];

          // Find the component array by type name
          for (const auto& [type_id, component_array] : Internal::component_arrays) {
            if (component_array->get_type_name() == type_name && component_array->is_reflectable()) {
              // Deserialize and add the component
              component_array->add_component_from_generic_value(entity.get_id(), component_data);
              Internal::entity_signatures[entity.get_id()].set(type_id);
              break;
            }
          }
        }

        // Notify systems about the component changes
        Internal::entity_component_changed(entity.get_id());
        };

      return std::make_shared<PrefabData>(setup_function);
    }

    std::string serialize_prefab_to_json(const Prefab& prefab) {
      if (!prefab || !prefab->setup_function) {
        return "{}";
      }
      ReflectionLibrary::GenericValue generic_value = to_generic_value(prefab);
      return ReflectionLibrary::generic_value_to_json(generic_value, true);
    }

    Prefab deserialize_prefab_from_json(const std::string& json_str) {
      std::optional<ReflectionLibrary::GenericValue> generic_value = ReflectionLibrary::generic_value_from_json(json_str);
      if (!generic_value) {
        return nullptr;
      }
      return from_generic_value(generic_value.value());
    }
  } // namespace ECS
} // namespace Engine
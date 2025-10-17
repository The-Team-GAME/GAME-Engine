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
#include "../Reflection/ReflectionLibrary.h"
#include "../Reflection/ReflectionLibrary/Json.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace Engine {
  namespace ECS {

    Prefab::Prefab(std::function<void(Entity&)> setup)
      : setup_function(std::move(setup)) {
    }

    Prefab create_prefab(std::function<void(Entity&)> setup) {
      return Prefab(std::move(setup));
    }

    Entity instantiate_prefab(const Prefab& prefab) {
      Entity entity = create_entity();
      if (prefab.setup_function) {
        prefab.setup_function(entity);
      }
      return entity;
    }

    std::string serialize_prefab_to_json(const Prefab& prefab) {
      if (!prefab.setup_function) {
        return "{}";
      }

      // Create a temporary entity and apply the prefab setup
      Entity temp_entity = create_entity();
      prefab.setup_function(temp_entity);

      rapidjson::Document doc;
      doc.SetObject();
      auto& allocator = doc.GetAllocator();

      // Create components array
      rapidjson::Value components_array(rapidjson::kArrayType);

      EntityID entity_id = temp_entity.get_id();
      const auto& signature = Internal::entity_signatures[entity_id];

      // Iterate through all component types and serialize the ones this entity has
      for (const auto& [type_id, component_array] : Internal::component_arrays) {
        if (signature.test(type_id) && component_array->is_reflectable()) {
          rapidjson::Value component_entry(rapidjson::kObjectType);

          // Add type name instead of type_id
          std::string type_name = component_array->get_type_name();
          rapidjson::Value type_name_val(type_name.c_str(), allocator);
          component_entry.AddMember("type", type_name_val, allocator);

          // Serialize component data using the component array's serialize method
          rapidjson::Value component_data(rapidjson::kObjectType);
          component_array->serialize_component(entity_id, component_data, allocator);
          component_entry.AddMember("data", component_data, allocator);

          components_array.PushBack(component_entry, allocator);
        }
      }

      doc.AddMember("components", components_array, allocator);

      // Clean up temporary entity
      destroy_entity(temp_entity);

      // Convert to string
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);

      return buffer.GetString();
    }

    Prefab deserialize_prefab_from_json(const std::string& json_str) {
      rapidjson::Document doc;
      doc.Parse(json_str.c_str());

      if (doc.HasParseError() || !doc.IsObject()) {
        return Prefab();
      }

      if (!doc.HasMember("components") || !doc["components"].IsArray()) {
        return Prefab();
      }

      // Create a lambda that will setup the entity with the deserialized components
      auto setup_function = [components_data = doc["components"].GetArray()](Entity& entity) {
        // Iterate through each component in the JSON
        for (const auto& component_entry : components_data) {
          if (!component_entry.IsObject() || !component_entry.HasMember("type") || !component_entry.HasMember("data")) {
            continue;
          }

          std::string type_name = component_entry["type"].GetString();
          const auto& component_data = component_entry["data"];

          // Find the component array by type name
          for (const auto& [type_id, component_array] : Internal::component_arrays) {
            if (component_array->get_type_name() == type_name && component_array->is_reflectable()) {
              // Deserialize and add the component
              component_array->deserialize_and_add_component(entity.get_id(), component_data);
              Internal::entity_signatures[entity.get_id()].set(type_id);
              break;
            }
          }
        }

        // Notify systems about the component changes
        Internal::entity_component_changed(entity.get_id());
      };

      return Prefab(setup_function);
    }

  } // namespace ECS
} // namespace Engine
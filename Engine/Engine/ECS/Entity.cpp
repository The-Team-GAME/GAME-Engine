//-----------------------------------------------------------------------------------
// file:    Entity.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Entity class methods for entity handle
//          management, validation, and comparison operations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Entity.h"
#include "World.h"
#include "WorldData.h"

namespace Engine {
  namespace ECS {

    Entity::Entity(EntityID id) : id(id) {}

    EntityID Entity::get_id() const {
      return id;
    }

    bool Entity::is_valid() const {
      return id != INVALID_ENTITY;
    }

    void Entity::destroy() {
      ECS::destroy_entity(*this);
    }

    bool Entity::operator==(const Entity& other) const {
      return id == other.id;
    }

    bool Entity::operator!=(const Entity& other) const {
      return id != other.id;
    }

    Entity create_entity_from_generic_value(const ReflectionLibrary::GenericValue& value) {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return Entity(INVALID_ENTITY);
      }

      Entity entity = ECS::create_entity();

      const auto& components_array = value["components"].getArray();
      auto& containers = Internal::get_containers();

      // Iterate through each component in the JSON
      for (const auto& component_entry : components_array) {
        if (!component_entry.isObject() || !component_entry.has("type") || !component_entry.has("data")) {
          continue;
        }

        std::string type_name = component_entry["type"].getString();
        const auto& component_data = component_entry["data"];

        // Find the component array by type name
        for (const auto& [type_id, component_array] : containers.component_arrays) {
          if (component_array->get_type_name() == type_name && component_array->is_reflectable()) {
            // Deserialize and add the component
            component_array->add_component_from_generic_value(entity.get_id(), component_data);
            containers.entity_signatures[entity.get_id()].set(type_id);
            break;
          }
        }
      }

      // Notify systems about the component changes
      Internal::entity_component_changed(entity.get_id());

      return entity;
    }

    ReflectionLibrary::GenericValue entity_to_generic_value(Entity entity) {
      auto& containers = Internal::get_containers();

      EntityID entity_id = entity.get_id();
      const auto& signature = containers.entity_signatures[entity_id];

      ReflectionLibrary::GenericValue result;
      result["id"] = entity_id;

      // Serialize each component's data
      for (const auto& [type_id, component_array] : containers.component_arrays) {
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

  } // namespace ECS
} // namespace Engine
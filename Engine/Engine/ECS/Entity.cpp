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

    void Entity::for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) {
      if (!is_valid())
        return;

      auto& containers = Internal::get_containers();
      auto sig_it = containers.entity_signatures.find(id);

      if (sig_it == containers.entity_signatures.end())
        return;

      const ComponentSignature& signature = sig_it->second;

      // Iterate through all component arrays
      for (auto& [type_id, array] : containers.component_arrays) {
        // Check if this entity has this component type
        if (signature.test(type_id)) {
          std::string type_name = array->get_type_name();
          callback(type_name, array.get());
        }
      }
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
        if (!component_entry.isObject() || !component_entry.has("type")) {
          continue;
        }

        std::string component_type = component_entry["type"].getString();

        // Find the component array for this type
        for (auto& [type_id, array] : containers.component_arrays) {
          if (array->get_type_name() == component_type) {
            array->add_component_from_generic_value(entity.get_id(), component_entry["data"]);
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
      result["dataType"] = "Entity";
      result["id"] = static_cast<int>(entity_id);

      ReflectionLibrary::Array components_array;

      // Iterate through all component arrays
      for (auto& [type_id, array] : containers.component_arrays) {
        if (!array->is_reflectable()) {
          continue; // Skip non-reflectable components
        }

        if (signature.test(type_id)) {
          ReflectionLibrary::GenericValue component_entry;
          component_entry["type"] = array->get_type_name();
          component_entry["data"] = array->to_generic_value(entity_id);
          components_array.push_back(component_entry);
        }
      }

      result["components"] = components_array;

      return result;
    }

  } // namespace ECS
} // namespace Engine
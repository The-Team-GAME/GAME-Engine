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
//          templates. Provides functions for creating and instantiating prefabs
//          by cloning component data.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Prefab.h"
#include "World.h"
#include "WorldData.h"
#include "ComponentArray.h"
#include "../Reflection/ReflectionLibrary.h"

namespace Engine {
  namespace ECS {

    // ============================================================================
    // PrefabImpl Implementation
    // ============================================================================

    const ComponentSignature& PrefabImpl::get_signature() const {
      return signature;
    }

    ComponentSignature& PrefabImpl::get_signature() {
      return signature;
    }

    const std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>>& PrefabImpl::get_component_storage() const {
      return component_storage;
    }

    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>>& PrefabImpl::get_component_storage() {
      return component_storage;
    }

    void PrefabImpl::for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) {
      for (auto& [type_id, array] : component_storage) {
        if (signature.test(type_id)) {
          callback(array->get_type_name(), array.get());
        }
      }
    }

    // ============================================================================
    // Prefab Handle Implementation
    // ============================================================================

    Prefab::Prefab(std::shared_ptr<PrefabImpl> impl) : impl(std::move(impl)) {}

    bool Prefab::is_valid() const {
      return impl != nullptr;
    }

    Prefab::operator bool() const {
      return impl != nullptr;
    }

    std::shared_ptr<PrefabImpl> Prefab::get_impl() const {
      return impl;
    }

    void Prefab::for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) {
      if (impl) {
        impl->for_each_component(callback);
      }
    }

    void Prefab::for_each_component(std::function<void(const std::string&, IComponentArray*)> callback) const {
      if (impl) {
        impl->for_each_component(callback);
      }
    }

    // ============================================================================
    // Prefab Factory Functions
    // ============================================================================

    Prefab create_prefab() {
      return Prefab(std::make_shared<PrefabImpl>());
    }

    Entity instantiate_prefab(const Prefab& prefab) {
      if (!prefab.is_valid()) {
        return Entity(INVALID_ENTITY);
      }

      // Create new entity
      Entity entity = create_entity();
      EntityID entity_id = entity.get_id();

      auto& containers = Internal::get_containers();
      auto prefab_impl = prefab.get_impl();

      // Copy signature
      containers.entity_signatures[entity_id] = prefab_impl->get_signature();

      // Clone all components from prefab to entity
      for (const auto& [type_id, prefab_array] : prefab_impl->get_component_storage()) {
        // Get or create component array in world
        auto it = containers.component_arrays.find(type_id);
        if (it == containers.component_arrays.end()) {
          // Component type not registered in world, create it
          containers.component_arrays[type_id] = prefab_array->create_empty();
          it = containers.component_arrays.find(type_id);
        }

        // Clone component from prefab's storage (entity ID 0) to world's storage (new entity_id)
        // We need to get the component data from prefab and add it to world
        ReflectionLibrary::GenericValue component_data = prefab_array->to_generic_value(0);
        it->second->add_component_from_generic_value(entity_id, component_data);
      }

      // Notify systems about the new entity
      Internal::entity_component_changed(entity_id);

      return entity;
    }

    Prefab entity_to_prefab(const Entity& entity) {
      if (!entity.is_valid()) {
        return Prefab();
      }

      auto& containers = Internal::get_containers();
      EntityID entity_id = entity.get_id();

      // Check if entity exists
      if (!containers.active_entities.contains(entity_id)) {
        return Prefab();
      }

      // Convert entity to generic value, then create prefab from that
      ReflectionLibrary::GenericValue entity_data = entity_to_generic_value(entity);

      if (!entity_data.has("components")) {
        return create_prefab(); // Return empty prefab
      }

      // Create prefab and populate it from the entity data
      return prefab_from_generic_value(entity_data);
    }    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab) {
      if (!prefab.is_valid()) {
        return ReflectionLibrary::GenericValue();
      }

      ReflectionLibrary::GenericValue result;
      result["dataType"] = "Prefab";

      ReflectionLibrary::Array components_array;

      prefab.for_each_component([&](const std::string& type_name, IComponentArray* array) {
        if (!array->is_reflectable()) {
          return;
        }

        ReflectionLibrary::GenericValue component_entry;
        component_entry["type"] = type_name;
        component_entry["data"] = array->to_generic_value(0); // Using dummy entity ID 0

        components_array.push_back(component_entry);
        });

      result["components"] = components_array;

      return result;
    }

    Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value) {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return Prefab();
      }

      Prefab prefab = create_prefab();
      auto prefab_impl = prefab.get_impl();

      const auto& components_array = value["components"].getArray();
      auto& containers = Internal::get_containers();

      // Get mutable access to prefab storage and signature
      auto& prefab_storage = prefab_impl->get_component_storage();
      auto& prefab_signature = prefab_impl->get_signature();

      // Iterate through each component in the JSON
      for (const auto& component_entry : components_array) {
        if (!component_entry.isObject() || !component_entry.has("type") || !component_entry.has("data")) {
          continue;
        }

        const std::string& component_type = component_entry["type"].getString();
        const auto& component_data = component_entry["data"];

        // Find matching component array type in world
        for (auto& [type_id, world_array] : containers.component_arrays) {
          if (world_array->get_type_name() == component_type) {
            // Create a new empty component array for the prefab
            if (prefab_storage.find(type_id) == prefab_storage.end()) {
              prefab_storage[type_id] = world_array->create_empty();
            }

            // Add component to prefab's storage using dummy entity ID 0
            prefab_storage[type_id]->add_component_from_generic_value(0, component_data);

            // Update prefab's signature
            prefab_signature.set(type_id);

            break;
          }
        }
      }

      return prefab;
    }
  } // namespace ECS
} // namespace Engine
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
#include "Reflection.h"
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

    const LayerSignature& PrefabImpl::get_layers() const {
      return layers;
    }

    LayerSignature& PrefabImpl::get_layers() {
      return layers;
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

    Entity Prefab::instantiate() const {
      return instantiate_prefab(*this);
    }

    Prefab& Prefab::add_to_layer(const std::string& layer_name) {
      if (!impl) return *this;

      LayerID layer = get_layer_id(layer_name);
      if (layer != INVALID_LAYER) {
        impl->get_layers().set(layer);
      }
      return *this;
    }

    Prefab& Prefab::remove_from_layer(const std::string& layer_name) {
      if (!impl) return *this;

      LayerID layer = get_layer_id(layer_name);
      if (layer != INVALID_LAYER) {
        impl->get_layers().reset(layer);
      }
      return *this;
    }

    Prefab& Prefab::clear_layers() {
      if (impl) {
        impl->get_layers().reset();
      }
      return *this;
    }

    LayerSignature Prefab::get_layers() const {
      if (impl) {
        return impl->get_layers();
      }
      return LayerSignature{};
    }

    // ============================================================================
    // Prefab Factory Functions
    // ============================================================================

    Prefab create_prefab() {
      auto prefab = Prefab(std::make_shared<PrefabImpl>());
      // Initialize prefab with default layer
      prefab.get_impl()->get_layers().set(DEFAULT_LAYER);
      return prefab;
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

      // Copy layers
      const LayerSignature& prefab_layers = prefab_impl->get_layers();
      entity.clear_layers();
      for (LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
        if (prefab_layers.test(layer)) {
          containers.entity_layers[entity_id].set(layer);
          Internal::add_entity_to_layer_set(entity_id, layer);
        }
      }

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
    }

    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab) {
      if (!prefab.is_valid()) {
        return ReflectionLibrary::GenericValue();
      }

      ReflectionLibrary::GenericValue result;
      // result["dataType"] = "Prefab";

      auto prefab_impl = prefab.get_impl();

      // Serialize layers and components using helper functions
      result["layers"] = Serialization::serialize_layers(prefab_impl->get_layers());
      result["components"] = Serialization::serialize_components(
        [&prefab](auto callback) { prefab.for_each_component(callback); },
        0 // Using dummy entity ID 0 for prefabs
      );

      return result;
    }

    Prefab prefab_from_generic_value(const ReflectionLibrary::GenericValue& value) {
      if (!value.isObject() || !value.has("components") || !value["components"].isArray()) {
        return Prefab();
      }

      Prefab prefab = create_prefab();
      auto prefab_impl = prefab.get_impl();

      // Deserialize layers using helper function
      LayerSignature& prefab_layers = prefab_impl->get_layers();
      prefab_layers = Serialization::deserialize_layers(value);

      // Deserialize components using helper function
      const auto& components_array = value["components"].getArray();
      auto& prefab_storage = prefab_impl->get_component_storage();
      auto& prefab_signature = prefab_impl->get_signature();

      Serialization::deserialize_components_to_storage(
        components_array,
        prefab_storage,
        prefab_signature,
        0 // Using dummy entity ID 0 for prefabs
      );

      return prefab;
    }
  } // namespace ECS
} // namespace Engine
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
#include "PrefabMergeUtils.h"
#include "../Resources.h"
#include "../Components/SpatialComponents.h"
#include "ComponentManager.h"
#include "../Random.h"
#include "../Graphics/Renderer.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"

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

    const std::string& PrefabImpl::get_prefab_id() const {
      return prefab_id;
    }

    void PrefabImpl::set_prefab_id(const std::string& id) {
      prefab_id = id;
    }

    const std::string& PrefabImpl::get_prefab_filepath() const {
      return prefab_filepath;
    }

    void PrefabImpl::set_prefab_filepath(const std::string& filepath) {
      prefab_filepath = filepath;
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

    bool Prefab::add_component_by_name(const std::string& component_name) {
      if (!impl)
        return false;

      auto& containers = Internal::get_containers();
      auto& prefab_storage = impl->get_component_storage();
      auto& prefab_signature = impl->get_signature();

      // Find the component array with matching type name
      for (auto& [type_id, world_array] : containers.component_arrays) {
        if (world_array->get_type_name() == component_name) {
          // Check if prefab already has this component
          if (prefab_signature.test(type_id)) {
            return false; // Already has this component
          }

          // Create or get component array in prefab storage
          if (prefab_storage.find(type_id) == prefab_storage.end()) {
            prefab_storage[type_id] = world_array->create_empty();
          }

          // Create a default-constructed component using GenericValue
          ReflectionLibrary::GenericValue default_value = ReflectionLibrary::Object();

          // Add the component to prefab (using entity ID 0 for prefabs)
          if (prefab_storage[type_id]->add_component_from_generic_value(0, default_value)) {
            prefab_signature.set(type_id);
            return true;
          }
          return false;
        }
      }

      return false; // Component type not found
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
      // Generate unique UUID for this prefab
      prefab.get_impl()->set_prefab_id(Random::generate_uuid());
      return prefab;
    }

    Entity instantiate_prefab(const Prefab& prefab) {
      if (!prefab.is_valid()) {
        return Entity(INVALID_ENTITY);
      }

      auto prefab_impl = prefab.get_impl();

      // Check for loading errors and warn the user
      if (prefab_impl->has_load_errors()) {
        const auto& errors = prefab_impl->get_load_errors();
        std::string error_msg = "Cannot instantiate prefab '" + prefab_impl->get_prefab_filepath() +
          "' due to loading errors:\n";
        for (size_t i = 0; i < errors.size() && i < 3; ++i) {
          error_msg += "- " + errors[i] + "\n";
        }
        if (errors.size() > 3) {
          error_msg += "... and " + std::to_string(errors.size() - 3) + " more errors";
        }
        Log::error("{}", error_msg);
        Engine::EventManager::send_event<Engine::Events::ToastEvent>(
          error_msg,
          Engine::Events::ToastType::Error,
          8.0f
        );
        return Entity(INVALID_ENTITY);
      }

      // Create new entity
      Entity entity = create_entity();
      EntityID entity_id = entity.get_id();

      auto& containers = Internal::get_containers();

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

      // Set Transform2D position to center of screen if entity has Transform2D
      if (entity.has_component<Components::Transform2D>()) {
        auto* transform = entity.get_component<Components::Transform2D>();
        if (transform) {
          // Get virtual resolution (screen center in virtual coordinates)
          Math::Vec2D virtual_resolution = Engine::Graphics::get_virtual_resolution();
          Math::Vec2D screen_center(virtual_resolution.x / 2.0f, virtual_resolution.y / 2.0f);

          // Convert screen center to world coordinates
          Math::Vec2D world_pos = Engine::Graphics::virtual_to_world(screen_center);
          transform->position = world_pos;
        }
      }

      // Track this as a prefab instance if the prefab has an ID
      const std::string& prefab_id = prefab_impl->get_prefab_id();
      if (!prefab_id.empty()) {
        Internal::PrefabInstanceData instance_data;
        instance_data.prefab_id = prefab_id; // Store UUID in prefab_id field

        // Mark Transform2D as fully overridden (Transform2D should never sync from prefab)
        auto transform_type_id = ComponentTypeManager::get_type_id<Components::Transform2D>();
        instance_data.field_overrides[transform_type_id].insert("*"); // "*" means all fields

        containers.prefab_instances[entity_id] = std::move(instance_data);
      }

      // Notify systems about the new entity
      Internal::entity_component_changed(entity_id);

      return entity;
    }

    Entity instantiate_prefab_with_id(const Prefab& prefab, EntityID entity_id) {
      if (!prefab.is_valid()) {
        return Entity(INVALID_ENTITY);
      }

      // Create entity with specific ID
      Entity entity = create_entity_with_id(entity_id);

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

        // Clone component from prefab's storage (entity ID 0) to world's storage (entity_id)
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
      Prefab prefab = prefab_from_generic_value(entity_data);

      // Register the entity as an instance of this prefab
      if (prefab.is_valid()) {
        const std::string& prefab_uuid = prefab.get_impl()->get_prefab_id();
        if (!prefab_uuid.empty()) {
          Internal::PrefabInstanceData instance_data;
          instance_data.prefab_id = prefab_uuid;

          // Mark Transform2D as fully overridden (Transform2D should never sync from prefab)
          auto transform_type_id = ComponentTypeManager::get_type_id<Components::Transform2D>();
          instance_data.field_overrides[transform_type_id].insert("*"); // "*" means all fields

          containers.prefab_instances[entity_id] = std::move(instance_data);
        }
      }

      return prefab;
    }

    ReflectionLibrary::GenericValue prefab_to_generic_value(const Prefab& prefab) {
      if (!prefab.is_valid()) {
        return ReflectionLibrary::GenericValue();
      }

      ReflectionLibrary::GenericValue result;
      // result["dataType"] = "Prefab";

      auto prefab_impl = prefab.get_impl();

      // Serialize UUID
      result["uuid"] = prefab_impl->get_prefab_id();

      // Serialize filepath
      result["filepath"] = prefab_impl->get_prefab_filepath();

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
        Log::warning("prefab_from_generic_value: Invalid value structure");
        return Prefab();
      }

      Prefab prefab = create_prefab();
      auto prefab_impl = prefab.get_impl();

      // Deserialize UUID if it exists, otherwise keep the generated one
      if (value.has("uuid") && value["uuid"].isString()) {
        prefab_impl->set_prefab_id(value["uuid"].getString());
        Log::info("Loaded prefab with UUID: {}", value["uuid"].getString());
      }

      // Deserialize filepath if it exists
      if (value.has("filepath") && value["filepath"].isString()) {
        prefab_impl->set_prefab_filepath(value["filepath"].getString());
      }

      // Deserialize layers using helper function
      LayerSignature& prefab_layers = prefab_impl->get_layers();
      prefab_layers = Serialization::deserialize_layers(value);

      // Deserialize components using helper function
      const auto& components_array = value["components"].getArray();
      Log::info("Deserializing {} components for prefab", components_array.size());

      auto& prefab_storage = prefab_impl->get_component_storage();
      auto& prefab_signature = prefab_impl->get_signature();

      auto deserialize_result = Serialization::deserialize_components_to_storage(
        components_array,
        prefab_storage,
        prefab_signature,
        0 // Using dummy entity ID 0 for prefabs
      );

      // Store any deserialization errors in the prefab
      if (deserialize_result.has_errors()) {
        for (const auto& error : deserialize_result.errors) {
          prefab_impl->add_load_error(error);
        }
        Log::warning("Prefab loaded with {} errors", deserialize_result.errors.size());
      }

      Log::info("Prefab now has {} components in storage", prefab_storage.size());

      return prefab;
    }

    // ============================================================================
    // Prefab Instance Management
    // ============================================================================

    void update_prefab_instances(const std::string& prefab_id) {
      auto& containers = Internal::get_containers();

      if (prefab_id.empty()) {
        return; // Can't update instances without UUID
      }

      // Find all instances of this prefab by UUID
      std::vector<EntityID> instances_to_update;
      for (const auto& [entity_id, instance_data] : containers.prefab_instances) {
        if (instance_data.prefab_id == prefab_id) { // Compare UUIDs
          instances_to_update.push_back(entity_id);
        }
      }

      // We need to get the prefab data to update instances
      // Get from registry if available
      Prefab prefab;
      for (const auto& [path, weak_prefab] : Engine::Resources::prefab_cache) {
        auto shared_prefab_impl = weak_prefab.lock();
        if (shared_prefab_impl && shared_prefab_impl->get_prefab_id() == prefab_id) {
          prefab = Prefab(shared_prefab_impl);
          break;
        }
      }

      if (!prefab.is_valid()) {
        return; // Prefab not found in cache
      }

      auto prefab_impl = prefab.get_impl();

      // Update each instance
      for (EntityID entity_id : instances_to_update) {
        if (!containers.active_entities.contains(entity_id)) {
          continue;
        }

        const auto& instance_data = containers.prefab_instances[entity_id];

        // For each component in the prefab
        for (const auto& [type_id, prefab_array] : prefab_impl->get_component_storage()) {
          // Check if component is fully overridden (marked with "*")
          auto override_it = instance_data.field_overrides.find(type_id);
          if (override_it != instance_data.field_overrides.end()) {
            if (override_it->second.count("*") > 0) {
              // Entire component is overridden, skip
              continue;
            }
          }

          // Get the current instance component data
          auto instance_array_it = containers.component_arrays.find(type_id);
          if (instance_array_it == containers.component_arrays.end()) {
            continue;
          }

          auto& instance_array = instance_array_it->second;

          // Get component data as GenericValues
          ReflectionLibrary::GenericValue prefab_component_data = prefab_array->to_generic_value(0);
          ReflectionLibrary::GenericValue instance_component_data = instance_array->to_generic_value(entity_id);

          // Get field overrides for this component
          std::unordered_set<std::string> overrides;
          if (override_it != instance_data.field_overrides.end()) {
            overrides = override_it->second;
          }

          // Merge prefab data with instance overrides
          ReflectionLibrary::GenericValue merged_data =
            PrefabMergeUtils::merge_component_with_overrides(
              prefab_component_data,
              instance_component_data,
              overrides
            );

          // Apply merged data back to instance
          instance_array->set_component_from_generic_value(entity_id, merged_data);
        }

        // Notify systems of component changes
        Internal::entity_component_changed(entity_id);
      }
    }

    void mark_field_overridden(Entity entity, ComponentTypeID type, const std::string& field_path) {
      if (!entity.is_valid()) return;

      auto& containers = Internal::get_containers();
      EntityID entity_id = entity.get_id();

      auto it = containers.prefab_instances.find(entity_id);
      if (it == containers.prefab_instances.end()) {
        // Not a prefab instance, nothing to do
        return;
      }

      it->second.field_overrides[type].insert(field_path);
    }

    void clear_field_override(Entity entity, ComponentTypeID type, const std::string& field_path) {
      if (!entity.is_valid()) return;

      auto& containers = Internal::get_containers();
      EntityID entity_id = entity.get_id();

      auto it = containers.prefab_instances.find(entity_id);
      if (it == containers.prefab_instances.end()) {
        return;
      }

      auto override_it = it->second.field_overrides.find(type);
      if (override_it != it->second.field_overrides.end()) {
        override_it->second.erase(field_path);

        // If no more overrides for this component, remove the entry
        if (override_it->second.empty()) {
          it->second.field_overrides.erase(override_it);
        }
      }
    }

    bool is_prefab_instance(Entity entity) {
      if (!entity.is_valid()) return false;

      auto& containers = Internal::get_containers();
      return containers.prefab_instances.find(entity.get_id()) != containers.prefab_instances.end();
    }

    std::string get_prefab_path(Entity entity) {
      if (!entity.is_valid()) return "";

      auto& containers = Internal::get_containers();
      auto it = containers.prefab_instances.find(entity.get_id());
      if (it == containers.prefab_instances.end()) {
        return ""; // Not a prefab instance
      }

      // Look up the prefab by UUID to find its filepath
      const std::string& prefab_id = it->second.prefab_id;
      for (const auto& [path, weak_prefab] : Engine::Resources::prefab_cache) {
        auto shared_prefab_impl = weak_prefab.lock();
        if (shared_prefab_impl && shared_prefab_impl->get_prefab_id() == prefab_id) {
          return shared_prefab_impl->get_prefab_filepath();
        }
      }

      return ""; // Prefab not found in cache
    }


  } // namespace ECS
} // namespace Engine
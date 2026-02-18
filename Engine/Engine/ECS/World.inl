//-----------------------------------------------------------------------------------
// file:    World.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for the World namespace.
//          Provides inline implementations for component management, system
//          registration, event handling, and entity queries.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "World.h"
#include "WorldData.h"

namespace Engine {
  namespace ECS
  {
    template <typename T>
    ComponentArray<T>* get_component_array()
    {
      auto& containers = Internal::get_containers();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto& ptr = containers.component_arrays[typeId];
      if (ptr)
      {
        return static_cast<ComponentArray<T> *>(ptr.get());
      }

      register_component<T>();
      return static_cast<ComponentArray<T> *>(containers.component_arrays[typeId].get());
    }

    template <typename T>
    void register_component()
    {
      auto& containers = Internal::get_containers();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      if (containers.component_arrays[typeId])
      {
        return;
      }
      containers.component_arrays[typeId] = std::make_unique<ComponentArray<T>>();
    }

    template <typename T>
    void unregister_component()
    {
      auto& containers = Internal::get_containers();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      containers.component_arrays[typeId].reset();
    }

    template <typename... T>
    void destroy_entities_with()
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<T>())), ...);

      // Create a vector copy to avoid iterator invalidation
      std::vector<EntityID> entities_to_check;
      entities_to_check.reserve(containers.active_entities.size());
      for (EntityID entityId : containers.active_entities)
      {
        entities_to_check.push_back(entityId);
      }

      // Now iterate over the copy and destroy matching entities
      for (EntityID entityId : entities_to_check)
      {
        if (entityId < containers.entity_signatures.size() &&
            (containers.entity_signatures[entityId] & required) == required)
        {
          destroy_entity(Entity(entityId));
        }
      }
    }

    template <typename T, typename... Args>
      requires std::is_constructible_v<T, Args...>
    void add_component(const Entity entity, Args &&...args)
    {
      auto& containers = Internal::get_containers();
      EntityID entityId = entity.get_id();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto* array = get_component_array<T>();

      array->add_component(entityId, T(std::forward<Args>(args)...));
      containers.entity_signatures[entityId].set(typeId);
      Internal::entity_component_changed(entityId);
    }

    template <typename T>
    void remove_component(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID entityId = entity.get_id();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();

      // Add to removal queue instead of removing immediately
      containers.components_to_remove.emplace_back(entityId, typeId);
    }

    template <typename T>
    T* get_component(const Entity entity)
    {
      EntityID entityId = entity.get_id();
      auto* array = get_component_array<T>();
      return array ? array->get_component(entityId) : nullptr;
    }

    template <typename T>
    bool has_component(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID entityId = entity.get_id();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      return entityId < containers.entity_signatures.size() && containers.entity_signatures[entityId].test(typeId);
    }

    template <typename... T>
    bool has_components(const Entity entity)
    {
      return (has_component<T>(entity) && ...);
    }

    template <typename T, typename... Args>
    T* add_system(Args &&...args)
    {
      auto& containers = Internal::get_containers();
      auto system = std::make_unique<T>(std::forward<Args>(args)...);
      T* ptr = system.get();

      for (EntityID entity : containers.active_entities)
      {
        if (Internal::system_matches_entity(*system, entity))
        {
          system->add_entity(entity);
        }
      }

      containers.systems.push_back(std::move(system));
      return ptr;
    }

    template <typename T>
    void remove_system()
    {
      auto& containers = Internal::get_containers();
      containers.systems.erase(
        std::remove_if(containers.systems.begin(), containers.systems.end(),
          [](const std::unique_ptr<System>& system)
          {
            return dynamic_cast<T*>(system.get()) != nullptr;
          }),
        containers.systems.end());
    }

    template <typename T>
    void request_system_update()
    {
      auto& containers = Internal::get_containers();
      for (auto& system : containers.systems)
      {
        if (dynamic_cast<T*>(system.get()) != nullptr)
        {
          containers.systems_to_update_once.insert(system.get());
          break;
        }
      }
    }

    template <typename... T>
    std::vector<Entity> get_entities_with()
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<T>())), ...);

      std::vector<Entity> result;
      for (EntityID entityId : containers.active_entities)
      {
        if (entityId < containers.entity_signatures.size() &&
            (containers.entity_signatures[entityId] & required) == required)
        {
          result.emplace_back(entityId);
        }
      }
      return result;
    }

    template <typename... T>
    std::vector<Entity> get_entities_with(const LayerSignature& layers)
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<T>())), ...);

      std::vector<Entity> result;

      // Iterate active entities and check if they match any requested layer
      for (EntityID entityId : containers.active_entities)
      {
        // Check if entity is on any of the requested layers
        auto layer_it = containers.entity_layers.find(entityId);
        if (layer_it == containers.entity_layers.end())
          continue;
        if ((layer_it->second & layers).none())
          continue;

        if (entityId < containers.entity_signatures.size() &&
            (containers.entity_signatures[entityId] & required) == required)
        {
          result.emplace_back(entityId);
        }
      }

      return result;
    }

    template <typename... T>
    Entity get_first_entity_with()
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<T>())), ...);

      for (EntityID entityId : containers.active_entities)
      {
        if (entityId < containers.entity_signatures.size() &&
            (containers.entity_signatures[entityId] & required) == required)
        {
          return Entity(entityId);
        }
      }
      return Entity(INVALID_ENTITY);
    }

    template <typename... T>
    void for_each_entity_with(const LayerSignature& layers, std::function<void(Entity)> callback)
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<T>())), ...);

      // Iterate active entities and check if they match any requested layer
      for (EntityID entityId : containers.active_entities)
      {
        // Check if entity is on any of the requested layers
        auto layer_it = containers.entity_layers.find(entityId);
        if (layer_it == containers.entity_layers.end())
          continue;
        if ((layer_it->second & layers).none())
          continue;

        if (entityId < containers.entity_signatures.size() &&
            (containers.entity_signatures[entityId] & required) == required)
        {
          callback(Entity(entityId));
        }
      }
    }

    template <typename T>
    size_t get_component_count()
    {
      auto* array = get_component_array<T>();
      return array ? array->size() : 0;
    }

    template <typename... Components>
    void for_each_entity_pair(std::function<void(Entity, Entity)> callback)
    {
      auto& containers = Internal::get_containers();

      // Build component signature for required components
      ComponentSignature required_signature;
      ((required_signature.set(ComponentTypeManager::get_type_id<Components>())), ...);

      // Track processed pairs to avoid duplicates
      std::unordered_set<EntityPairHash> processed_pairs;

      // Iterate through all active entities as LHS
      for (size_t i = 0; i < containers.active_entities.size(); ++i)
      {
        EntityID lhs_id = containers.active_entities.key_at(i);
        if (lhs_id >= containers.entity_signatures.size())
          continue;

        // Check if LHS has all required components
        const auto& lhs_sig = containers.entity_signatures[lhs_id];
        if ((lhs_sig & required_signature) != required_signature)
          continue;

        // Iterate through all active entities as RHS
        for (size_t j = 0; j < containers.active_entities.size(); ++j)
        {
          EntityID rhs_id = containers.active_entities.key_at(j);
          if (lhs_id == rhs_id) continue; // Skip self-pairs

          if (rhs_id >= containers.entity_signatures.size())
            continue;

          // Check if RHS has all required components
          const auto& rhs_sig = containers.entity_signatures[rhs_id];
          if ((rhs_sig & required_signature) != required_signature)
            continue;

          // Create unique pair ID (order-independent hash)
          // EntityPairHash pair_hash = (static_cast<EntityPairHash>(std::min(lhs_id, rhs_id)) << 32) |
          //   static_cast<EntityPairHash>(std::max(lhs_id, rhs_id));

          // Order dependent version
          EntityPairHash pair_hash = (static_cast<EntityPairHash>(lhs_id) << 32) |
            static_cast<EntityPairHash>(rhs_id);

          // Skip if we've already processed this pair
          if (!processed_pairs.insert(pair_hash).second)
            continue;

          Entity lhs(lhs_id);
          Entity rhs(rhs_id);
          callback(lhs, rhs);
        }
      }
    }

    template <typename... Components>
    void for_each_entity_pair(
      const LayerSignature& lhs_mask,
      const LayerSignature& rhs_mask,
      std::function<void(Entity, Entity)> callback)
    {
      auto& containers = Internal::get_containers();

      // Build component signature for required components
      ComponentSignature required_signature;
      ((required_signature.set(ComponentTypeManager::get_type_id<Components>())), ...);

      // Track processed pairs to avoid duplicates
      std::unordered_set<EntityPairHash> processed_pairs;

      // Iterate through LHS layers that match the mask
      for (LayerID lhs_layer = 0; lhs_layer < containers.layer_entities.size(); ++lhs_layer) {
        if (!lhs_mask.test(lhs_layer)) continue;

        const auto& lhs_layer_set = containers.layer_entities[lhs_layer];

        // Iterate through RHS layers that match the mask
        for (LayerID rhs_layer = 0; rhs_layer < containers.layer_entities.size(); ++rhs_layer) {
          if (!rhs_mask.test(rhs_layer)) continue;

          const auto& rhs_layer_set = containers.layer_entities[rhs_layer];

          // Iterate all entity pairs between these two layers
          for (EntityID lhs_id : lhs_layer_set) {
            // Check if LHS has all required components
            if (lhs_id >= containers.entity_signatures.size())
              continue;

            const auto& lhs_sig = containers.entity_signatures[lhs_id];
            if ((lhs_sig & required_signature) != required_signature)
              continue;

            for (EntityID rhs_id : rhs_layer_set) {
              if (lhs_id == rhs_id) continue; // Skip self-pairs

              // Check if RHS has all required components
              if (rhs_id >= containers.entity_signatures.size())
                continue;

              const auto& rhs_sig = containers.entity_signatures[rhs_id];
              if ((rhs_sig & required_signature) != required_signature)
                continue;

              // Create unique pair ID (order-independent hash)
              EntityPairHash pair_hash = (static_cast<EntityPairHash>(std::min(lhs_id, rhs_id)) << 32) |
                static_cast<EntityPairHash>(std::max(lhs_id, rhs_id));

              // Skip if we've already processed this pair
              if (!processed_pairs.insert(pair_hash).second) continue;

              Entity lhs(lhs_id);
              Entity rhs(rhs_id);
              callback(lhs, rhs);
            }
          }
        }
      }
    }

    //template <typename... Components>
    //void for_each_entity_with(std::function<void(Entity)> callback)
    //{
    //  auto& containers = Internal::get_containers();

    //  // Build component signature for required components
    //  ComponentSignature required_signature;
    //  ((required_signature.set(ComponentTypeManager::get_type_id<Components>())), ...);

    //  // Iterate through all active entities
    //  for (size_t i = 0; i < containers.active_entities.size(); ++i)
    //  {
    //    EntityID id = containers.active_entities.key_at(i);
    //    auto it = containers.entity_signatures.find(id);
    //    if (it != containers.entity_signatures.end())
    //    {
    //      const auto& entity_sig = it->second;
    //      // Check if entity has all required components
    //      if ((entity_sig & required_signature) == required_signature)
    //      {
    //        Entity entity(id);
    //        callback(entity);
    //      }
    //    }
    //  }
    //}

    template <typename... T>
    void for_each_entity_with(std::function<void(Entity)> callback)
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<T>())), ...);

      // Iterate active entities directly — no duplicates, no allocation
      for (EntityID id : containers.active_entities) {
        if (id < containers.entity_signatures.size() &&
            (containers.entity_signatures[id] & required) == required) {
          callback(Entity(id));
        }
      }
    }

    // ============================================================================
    // for_each - Convenient iteration with direct component access
    // ============================================================================

    namespace Internal {
      // Helper to detect if callback wants Entity as first argument
      template<typename Func, typename Entity, typename... Components>
      concept CallbackWithEntity = requires(Func f, Entity e, Components*... comps) {
        f(e, comps...);
      };

      template<typename Func, typename... Components>
      concept CallbackWithoutEntity = requires(Func f, Components*... comps) {
        f(comps...);
      };
    }

    template <typename... Components, typename Func>
    void for_each(Func&& callback)
    {
      auto& containers = Internal::get_containers();
      // Precompute required signature once
      ComponentSignature required;
      ((required.set(ComponentTypeManager::get_type_id<Components>())), ...);

      // Cache component array pointers (one lookup each, outside the loop)
      auto arrays = std::make_tuple(get_component_array<Components>()...);

      // Iterate active entities directly — no duplicates, no allocation
      for (EntityID id : containers.active_entities) {
        if (id >= containers.entity_signatures.size() ||
            (containers.entity_signatures[id] & required) != required)
          continue;

        Entity entity(id);

        // Get all component pointers using cached arrays
        if constexpr (Internal::CallbackWithEntity<Func, Entity, Components...>) {
          callback(entity, std::get<ComponentArray<Components>*>(arrays)->get_component(id)...);
        }
        else if constexpr (Internal::CallbackWithoutEntity<Func, Components...>) {
          callback(std::get<ComponentArray<Components>*>(arrays)->get_component(id)...);
        }
        else {
          static_assert(sizeof(Func) == 0,
            "for_each callback must have signature void(Entity, Components*...) or void(Components*...)");
        }
      }
    }

    // ============================================================================
    // Layer Management Template Implementation
    // ============================================================================

    template<LayerID... Layers>
    void add_to_layers(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      if (!containers.active_entities.contains(id))
        return;

      auto& layers = containers.entity_layers[id];

      // Add to each layer's sparse set
      (([&]() {
        if (!layers.test(Layers)) {
          layers.set(Layers);
          Internal::add_entity_to_layer_set(id, Layers);
        }
        }()), ...);

      // Notify systems about layer change
      Internal::entity_component_changed(id);
    }

    template<LayerID... Layers>
    void remove_from_layers(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      if (!containers.active_entities.contains(id))
        return;

      auto& layers = containers.entity_layers[id];

      // Remove from each layer's sparse set
      (([&]() {
        if (layers.test(Layers)) {
          layers.reset(Layers);
          Internal::remove_entity_from_layer_set(id, Layers);
        }
        }()), ...);

      // Notify systems about layer change
      Internal::entity_component_changed(id);
    }

    template<LayerID... Layers>
    void set_layers(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      if (!containers.active_entities.contains(id))
        return;

      auto& entity_layers = containers.entity_layers[id];

      // Remove entity from all current layer sets
      for (LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
        if (entity_layers.test(layer)) {
          Internal::remove_entity_from_layer_set(id, layer);
        }
      }

      // Reset and set new layers
      entity_layers.reset();
      ((entity_layers.set(Layers)), ...);

      // Add entity to new layer sets
      (Internal::add_entity_to_layer_set(id, Layers), ...);

      // Notify systems about layer change
      Internal::entity_component_changed(id);
    }

    template<LayerID... Layers>
    bool is_on_any_layer(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      auto it = containers.entity_layers.find(id);
      if (it == containers.entity_layers.end())
        return false;

      LayerSignature check_layers;
      ((check_layers.set(Layers)), ...);

      return (it->second & check_layers).any();
    }

    template<LayerID... Layers>
    bool is_on_all_layers(const Entity entity)
    {
      auto& containers = Internal::get_containers();
      EntityID id = entity.get_id();

      auto it = containers.entity_layers.find(id);
      if (it == containers.entity_layers.end())
        return false;

      LayerSignature check_layers;
      ((check_layers.set(Layers)), ...);

      return (it->second & check_layers) == check_layers;
    }

  } // namespace ECS
} // namespace Engine
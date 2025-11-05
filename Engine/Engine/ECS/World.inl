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
      auto it = containers.component_arrays.find(typeId);
      if (it != containers.component_arrays.end())
      {
        return static_cast<ComponentArray<T> *>(it->second.get());
      }

      register_component<T>();
      return static_cast<ComponentArray<T> *>(containers.component_arrays[typeId].get());
    }

    template <typename T>
    void register_component()
    {
      auto& containers = Internal::get_containers();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      if (containers.component_arrays.find(typeId) != containers.component_arrays.end())
      {
        return;
      }
      containers.component_arrays[typeId] = std::make_unique<ComponentArray<T>>();
    }

    template <typename... T>
    void destroy_entities_with()
    {
      auto& containers = Internal::get_containers();
      for (EntityID entityId : containers.active_entities)
      {
        Entity entity(entityId);
        if (has_components<T...>(entity))
        {
          destroy_entity(entity);
        }
      }
    }

    template <typename T, typename... Args>
      requires std::is_constructible_v<T, Args...>
    void add_component(const Entity& entity, Args &&...args)
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
    void remove_component(const Entity& entity)
    {
      auto& containers = Internal::get_containers();
      EntityID entityId = entity.get_id();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();

      // Add to removal queue instead of removing immediately
      containers.components_to_remove.emplace_back(entityId, typeId);
    }

    template <typename T>
    T* get_component(const Entity& entity)
    {
      EntityID entityId = entity.get_id();
      auto* array = get_component_array<T>();
      return array ? array->get_component(entityId) : nullptr;
    }

    template <typename T>
    bool has_component(const Entity& entity)
    {
      auto& containers = Internal::get_containers();
      EntityID entityId = entity.get_id();
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto it = containers.entity_signatures.find(entityId);
      return it != containers.entity_signatures.end() && it->second.test(typeId);
    }

    template <typename... T>
    bool has_components(const Entity& entity)
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

    template <typename... T>
    std::vector<Entity> get_entities_with()
    {
      auto& containers = Internal::get_containers();
      std::vector<Entity> result;
      for (EntityID entityId : containers.active_entities)
      {
        if (has_components<T...>(entityId))
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
      std::vector<Entity> result;
      std::unordered_set<EntityID> processed; // Track processed entities to avoid duplicates

      // Iterate through each layer in the signature
      for (LayerID layer = 0; layer < MAX_LAYERS; ++layer)
      {
        if (!layers.test(layer))
          continue;

        // Check if this layer has any entities
        if (layer >= containers.layer_entities.size())
          continue;

        // Iterate through entities on this layer
        for (EntityID entityId : containers.layer_entities[layer])
        {
          // Skip if already processed or doesn't have required components
          if (processed.count(entityId) || !has_components<T...>(entityId))
            continue;

          processed.insert(entityId);
          result.emplace_back(entityId);
        }
      }

      return result;
    }

    template <typename... T>
    Entity get_first_entity_with()
    {
      auto& containers = Internal::get_containers();
      for (EntityID entityId : containers.active_entities)
      {
        if (has_components<T...>(entityId))
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
      std::unordered_set<EntityID> processed; // Track processed entities to avoid duplicates

      // Iterate through each layer in the signature
      for (LayerID layer = 0; layer < MAX_LAYERS; ++layer)
      {
        if (!layers.test(layer))
          continue;

        // Check if this layer has any entities
        if (layer >= containers.layer_entities.size())
          continue;

        // Iterate through entities on this layer
        for (EntityID entityId : containers.layer_entities[layer])
        {
          // Skip if already processed or doesn't have required components
          if (processed.count(entityId) || !has_components<T...>(entityId))
            continue;

          processed.insert(entityId);
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
        EntityID lhs_id = containers.active_entities.entity_at(i);
        auto lhs_it = containers.entity_signatures.find(lhs_id);
        if (lhs_it == containers.entity_signatures.end())
          continue;

        // Check if LHS has all required components
        const auto& lhs_sig = lhs_it->second;
        if ((lhs_sig & required_signature) != required_signature)
          continue;

        // Iterate through all active entities as RHS
        for (size_t j = 0; j < containers.active_entities.size(); ++j)
        {
          EntityID rhs_id = containers.active_entities.entity_at(j);
          if (lhs_id == rhs_id) continue; // Skip self-pairs

          auto rhs_it = containers.entity_signatures.find(rhs_id);
          if (rhs_it == containers.entity_signatures.end())
            continue;

          // Check if RHS has all required components
          const auto& rhs_sig = rhs_it->second;
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
            auto lhs_it = containers.entity_signatures.find(lhs_id);
            if (lhs_it == containers.entity_signatures.end())
              continue;

            const auto& lhs_sig = lhs_it->second;
            if ((lhs_sig & required_signature) != required_signature)
              continue;

            for (EntityID rhs_id : rhs_layer_set) {
              if (lhs_id == rhs_id) continue; // Skip self-pairs

              // Check if RHS has all required components
              auto rhs_it = containers.entity_signatures.find(rhs_id);
              if (rhs_it == containers.entity_signatures.end())
                continue;

              const auto& rhs_sig = rhs_it->second;
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
    //    EntityID id = containers.active_entities.entity_at(i);
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

      // Use a set to track already processed entities to avoid duplicates
      // since entities can be on multiple layers
      std::unordered_set<EntityID> processed;

      // Iterate through layers in order (lower layer ID = higher priority = processed first)
      for (LayerID layer = 0; layer < containers.layer_entities.size(); ++layer) {
        for (EntityID id : containers.layer_entities[layer]) {
          // Skip if already processed, not active, or doesn't have required components
          if (processed.count(id) || !containers.active_entities.contains(id) || !has_components<T...>(id))
            continue;

          processed.insert(id);
          Entity entity(id);
          callback(entity);
        }
      }
    }

    // ============================================================================
    // Layer Management Template Implementation
    // ============================================================================

    template<LayerID... Layers>
    void add_to_layers(const Entity& entity)
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
    void remove_from_layers(const Entity& entity)
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
    void set_layers(const Entity& entity)
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
    bool is_on_any_layer(const Entity& entity)
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
    bool is_on_all_layers(const Entity& entity)
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
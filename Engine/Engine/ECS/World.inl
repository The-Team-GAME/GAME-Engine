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
          // Check if entity has all required components
          if (has_components<T...>(entityId))
          {
            result.emplace_back(entityId);
          }
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

    template <typename T>
    size_t get_component_count()
    {
      auto* array = get_component_array<T>();
      return array ? array->size() : 0;
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
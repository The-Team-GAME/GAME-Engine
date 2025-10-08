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

namespace Engine {
  namespace ECS
  {
    template <typename T>
    ComponentArray<T>* get_component_array()
    {
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto it = Internal::component_arrays.find(typeId);
      if (it != Internal::component_arrays.end())
      {
        return static_cast<ComponentArray<T> *>(it->second.get());
      }

      register_component<T>();
      return static_cast<ComponentArray<T> *>(Internal::component_arrays[typeId].get());
    }

    template <typename T>
    void register_component()
    {
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      assert(Internal::component_arrays.find(typeId) == Internal::component_arrays.end() &&
        "Component already registered");
      Internal::component_arrays[typeId] = std::make_unique<ComponentArray<T>>();
    }

    template <typename... T>
    void destroy_entities_with()
    {
      for (EntityID entity : Internal::active_entities)
      {
        if (has_components<T...>(entity))
        {
          destroy_entity(entity);
        }
      }
    }

    template <typename T, typename... Args>
    void add_component(EntityID entity, Args &&...args)
    {
      assert(!has_component<T>(entity) && "Entity already has this component");
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto* array = get_component_array<T>();

      array->add_component(entity, T(std::forward<Args>(args)...));
      Internal::entity_signatures[entity].set(typeId);
      Internal::entity_component_changed(entity);
    }

    template <typename T>
    void remove_component(EntityID entity)
    {
      //assert(has_component<T>(entity) && "Entity does not have this component");
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto* array = get_component_array<T>();
      if (array)
      {
        array->remove_component(entity);
        Internal::entity_signatures[entity].reset(typeId);
        Internal::entity_component_changed(entity);
      }
    }

    template <typename T>
    T* get_component(EntityID entity)
    {
      //assert(has_component<T>(entity) && "Entity does not have this component");
      auto* array = get_component_array<T>();
      return array ? array->get_component(entity) : nullptr;
    }

    template <typename T>
    bool has_component(EntityID entity)
    {
      ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
      auto it = Internal::entity_signatures.find(entity);
      return it != Internal::entity_signatures.end() && it->second.test(typeId);
    }

    template <typename... T>
    bool has_components(EntityID entity)
    {
      return (has_component<T>(entity) && ...);
    }

    template <typename T, typename... Args>
    T* add_system(Args &&...args)
    {
      auto system = std::make_unique<T>(std::forward<Args>(args)...);
      T* ptr = system.get();

      for (EntityID entity : Internal::active_entities)
      {
        if (Internal::system_matches_entity(*system, entity))
        {
          system->add_entity(entity);
        }
      }

      Internal::systems.push_back(std::move(system));
      return ptr;
    }

    template <typename T>
    void remove_system()
    {
      Internal::systems.erase(
        std::remove_if(Internal::systems.begin(), Internal::systems.end(),
          [](const std::unique_ptr<System>& system)
          {
            return dynamic_cast<T*>(system.get()) != nullptr;
          }),
        Internal::systems.end());
    }

    template <typename T, typename... Args>
    void send_event(Args&&... args)
    {
      Internal::event_queue.emplace_back(T{ std::forward<Args>(args)... });
    }

    template <typename T>
    void add_event_listener(std::function<void(const T&)> handler)
    {
      EventTypeID eventType = EventTypeManager::get_type_id<T>();
      Internal::event_handlers[eventType].emplace_back([handler](const Event& event) {
        if (const T* eventData = event.get<T>()) {
          handler(*eventData);
        }
        });
    }

    template <typename T>
    void add_event_listener(std::function<void(const Event&)> handler)
    {
      EventTypeID eventType = EventTypeManager::get_type_id<T>();
      Internal::event_handlers[eventType].emplace_back(handler);
    }

    template <typename T>
    void remove_event_listener()
    {
      EventTypeID eventType = EventTypeManager::get_type_id<T>();
      remove_event_listener(eventType);
    }

    template <typename... T>
    std::vector<Entity> get_entities_with()
    {
      std::vector<Entity> result;
      for (EntityID entityId : Internal::active_entities)
      {
        if (has_components<T...>(entityId))
        {
          result.emplace_back(entityId);
        }
      }
      return result;
    }

    template <typename... T>
    Entity get_first_entity_with()
    {
      for (EntityID entityId : Internal::active_entities)
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
  } // namespace ECS
} // namespace Engine
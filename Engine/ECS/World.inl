//-----------------------------------------------------------------------------------
// file:    World.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for the World class.
//          Provides inline implementations for component management, system
//          registration, event handling, and entity queries.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "World.h"

namespace ECS
{

  template <typename T>
  ComponentArray<T>* World::get_component_array()
  {
    ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
    auto it = component_arrays.find(typeId);
    if (it != component_arrays.end())
    {
      return static_cast<ComponentArray<T> *>(it->second.get());
    }

    // If it doesn't exist, create and register it automatically.
    register_component<T>();
    return static_cast<ComponentArray<T> *>(component_arrays[typeId].get());
  }

  template <typename T>
  void World::register_component()
  {
    ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
    assert(component_arrays.find(typeId) == component_arrays.end() &&
      "Component already registered");
    component_arrays[typeId] = std::make_unique<ComponentArray<T>>();
  }

  template <typename... T>
  void World::destroy_entities_with()
  {
    for (EntityID entity : active_entities)
    {
      if (has_components<T...>(entity))
      {
        destroy_entity(entity);
      }
    }
  }

  template <typename T, typename... Args>
  void World::add_component(EntityID entity, Args &&...args)
  {
    assert(!has_component<T>(entity) && "Entity already has this component");
    ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
    auto* array = get_component_array<T>();
    // The assert is no longer strictly necessary as get_component_array now ensures creation.
    // assert(array && "Component not registered");


    // array->add_component(entity, T{ std::forward<Args>(args)... });
    // Use parentheses instead of braces to allow implicit conversions
    array->add_component(entity, T(std::forward<Args>(args)...));
    entity_signatures[entity].set(typeId);
    entity_component_changed(entity);
  }

  template <typename T>
  void World::remove_component(EntityID entity)
  {
    assert(has_component<T>(entity) && "Entity does not have this component");
    ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
    auto* array = get_component_array<T>();
    if (array)
    {
      array->remove_component(entity);
      entity_signatures[entity].reset(typeId);
      entity_component_changed(entity);
    }
  }

  template <typename T>
  T* World::get_component(EntityID entity)
  {
    assert(has_component<T>(entity) && "Entity does not have this component");
    auto* array = get_component_array<T>();
    return array ? array->get_component(entity) : nullptr;
  }

  template <typename T>
  const T* World::get_component(EntityID entity) const
  {
    assert(has_component<T>(entity) && "Entity does not have this component");
    auto* array = const_cast<World*>(this)->get_component_array<T>();
    return array ? array->get_component(entity) : nullptr;
  }

  template <typename T>
  bool World::has_component(EntityID entity) const
  {
    ComponentTypeID typeId = ComponentTypeManager::get_type_id<T>();
    auto it = entity_signatures.find(entity);
    return it != entity_signatures.end() && it->second.test(typeId);
  }

  template <typename... T>
  bool World::has_components(EntityID entity) const
  {
    return (has_component<T>(entity) && ...);
  }

  template <typename T, typename... Args>
  T* World::add_system(Args &&...args)
  {
    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = system.get();
    system->set_world(this);

    // Add existing matching entities to the system
    for (EntityID entity : active_entities)
    {
      if (system_matches_entity(*system, entity))
      {
        system->add_entity(entity);
      }
    }

    systems.push_back(std::move(system));
    return ptr;
  }

  template <typename T>
  void World::remove_system()
  {
    systems.erase(
      std::remove_if(systems.begin(), systems.end(),
        [](const std::unique_ptr<System>& system)
        {
          return dynamic_cast<T*>(system.get()) != nullptr;
        }),
      systems.end());
  }

  template <typename T, typename... Args>
  void World::send_event(Args&&... args)
  {
    event_queue.emplace_back(T{ std::forward<Args>(args)... });
  }

  template <typename T>
  void World::add_event_listener(std::function<void(const T&)> handler)
  {
    EventTypeID eventType = EventTypeManager::get_type_id<T>();
    event_handlers[eventType].emplace_back([handler](const Event& event) {
      if (const T* eventData = event.get<T>()) {
        handler(*eventData);
      }
      });
  }

  template <typename T>
  void World::add_event_listener(std::function<void(const Event&)> handler)
  {
    EventTypeID eventType = EventTypeManager::get_type_id<T>();
    event_handlers[eventType].emplace_back(handler);
  }

  template <typename T>
  void World::remove_event_listener()
  {
    EventTypeID eventType = EventTypeManager::get_type_id<T>();
    remove_event_listener(eventType);
  }

  template <typename... T>
  std::vector<Entity> World::get_entities_with() const
  {
    std::vector<Entity> result;
    for (EntityID entityId : active_entities)
    {
      if (has_components<T...>(entityId))
      {
        result.emplace_back(entityId, const_cast<World*>(this));
      }
    }
    return result;
  }

  template <typename... T>
  Entity World::get_first_entity_with() const
  {
    for (EntityID entityId : active_entities)
    {
      if (has_components<T...>(entityId))
      {
        return Entity(entityId, const_cast<World*>(this));
      }
    }
    return Entity(INVALID_ENTITY, const_cast<World*>(this));
  }

  template <typename T>
  size_t World::get_component_count() const
  {
    auto* array = const_cast<World*>(this)->get_component_array<T>();
    return array ? array->size() : 0;
  }

} // namespace ECS
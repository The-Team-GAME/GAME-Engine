#pragma once
#include "World.h"

namespace ECS
{

  template <typename T>
  ComponentArray<T>* World::get_component_array()
  {
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
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
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    assert(component_arrays.find(typeId) == component_arrays.end() &&
      "Component already registered");
    component_arrays[typeId] = std::make_unique<ComponentArray<T>>();
  }

  template <typename... T>
  void World::destroyEntitiesWith()
  {
    for (EntityID entity : active_entities)
    {
      if (hasComponents<T...>(entity))
      {
        destory_entity(entity);
      }
    }
  }

  template <typename T, typename... Args>
  void World::addComponent(EntityID entity, Args &&...args)
  {
    assert(!hasComponent<T>(entity) && "Entity already has this component");
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto* array = get_component_array<T>();
    // The assert is no longer strictly necessary as get_component_array now ensures creation.
    // assert(array && "Component not registered");

    array->addComponent(entity, T{ std::forward<Args>(args)... });
    entity_signatures[entity].set(typeId);
    entityComponentChanged(entity);
  }

  template <typename T>
  void World::removeComponent(EntityID entity)
  {
    assert(hasComponent<T>(entity) && "Entity does not have this component");
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto* array = get_component_array<T>();
    if (array)
    {
      array->removeComponent(entity);
      entity_signatures[entity].reset(typeId);
      entityComponentChanged(entity);
    }
  }

  template <typename T>
  T* World::get_component(EntityID entity)
  {
    assert(hasComponent<T>(entity) && "Entity does not have this component");
    auto* array = get_component_array<T>();
    return array ? array->get_component(entity) : nullptr;
  }

  template <typename T>
  const T* World::get_component(EntityID entity) const
  {
    assert(hasComponent<T>(entity) && "Entity does not have this component");
    auto* array = const_cast<World*>(this)->get_component_array<T>();
    return array ? array->get_component(entity) : nullptr;
  }

  template <typename T>
  bool World::hasComponent(EntityID entity) const
  {
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto it = entity_signatures.find(entity);
    return it != entity_signatures.end() && it->second.test(typeId);
  }

  template <typename... T>
  bool World::hasComponents(EntityID entity) const
  {
    return (hasComponent<T>(entity) && ...);
  }

  template <typename T, typename... Args>
  T* World::addSystem(Args &&...args)
  {
    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = system.get();
    system->setWorld(this);

    // Add existing matching entities to the system
    for (EntityID entity : active_entities)
    {
      if (systemMatchesEntity(*system, entity))
      {
        system->addEntity(entity);
      }
    }

    systems.push_back(std::move(system));
    return ptr;
  }

  template <typename T>
  void World::removeSystem()
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
  void World::sendEvent(Args&&... args)
  {
    eventQueue.emplace_back(T{ std::forward<Args>(args)... });
  }

  template <typename T>
  void World::addEventListener(std::function<void(const T&)> handler)
  {
    EventTypeID eventType = EventTypeManager::getTypeId<T>();
    eventHandlers[eventType].emplace_back([handler](const Event& event) {
      if (const T* eventData = event.get<T>()) {
        handler(*eventData);
      }
      });
  }

  template <typename T>
  void World::addEventListener(std::function<void(const Event&)> handler)
  {
    EventTypeID eventType = EventTypeManager::getTypeId<T>();
    eventHandlers[eventType].emplace_back(handler);
  }

  template <typename T>
  void World::removeEventListener()
  {
    EventTypeID eventType = EventTypeManager::getTypeId<T>();
    removeEventListener(eventType);
  }

  template <typename... T>
  std::vector<Entity> World::getEntitiesWith() const
  {
    std::vector<Entity> result;
    for (EntityID entityId : active_entities)
    {
      if (hasComponents<T...>(entityId))
      {
        result.emplace_back(entityId, const_cast<World*>(this));
      }
    }
    return result;
  }

  template <typename... T>
  Entity World::getFirstEntityWith() const
  {
    for (EntityID entityId : active_entities)
    {
      if (hasComponents<T...>(entityId))
      {
        return Entity(entityId, const_cast<World*>(this));
      }
    }
    return Entity(INVALID_ENTITY, const_cast<World*>(this));
  }

  template <typename T>
  size_t World::get_componentCount() const
  {
    auto* array = const_cast<World*>(this)->get_component_array<T>();
    return array ? array->size() : 0;
  }

} // namespace ECS
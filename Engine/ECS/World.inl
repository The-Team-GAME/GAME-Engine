#pragma once
#include "World.h"

namespace ECS
{

  template <typename T>
  ComponentArray<T>* World::get_componentArray()
  {
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto it = componentArrays.find(typeId);
    if (it != componentArrays.end())
    {
      return static_cast<ComponentArray<T> *>(it->second.get());
    }

    // If it doesn't exist, create and register it automatically.
    registerComponent<T>();
    return static_cast<ComponentArray<T> *>(componentArrays[typeId].get());
  }

  template <typename T>
  void World::registerComponent()
  {
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    assert(componentArrays.find(typeId) == componentArrays.end() &&
      "Component already registered");
    componentArrays[typeId] = std::make_unique<ComponentArray<T>>();
  }

  template <typename... T>
  void World::destroyEntitiesWith()
  {
    for (EntityID entity : activeEntities)
    {
      if (hasComponents<T...>(entity))
      {
        destroyEntity(entity);
      }
    }
  }

  template <typename T, typename... Args>
  void World::addComponent(EntityID entity, Args &&...args)
  {
    assert(!hasComponent<T>(entity) && "Entity already has this component");
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto* array = get_componentArray<T>();
    // The assert is no longer strictly necessary as get_componentArray now ensures creation.
    // assert(array && "Component not registered");

    array->addComponent(entity, T{ std::forward<Args>(args)... });
    entitySignatures[entity].set(typeId);
    entityComponentChanged(entity);
  }

  template <typename T>
  void World::removeComponent(EntityID entity)
  {
    assert(hasComponent<T>(entity) && "Entity does not have this component");
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto* array = get_componentArray<T>();
    if (array)
    {
      array->removeComponent(entity);
      entitySignatures[entity].reset(typeId);
      entityComponentChanged(entity);
    }
  }

  template <typename T>
  T* World::get_component(EntityID entity)
  {
    assert(hasComponent<T>(entity) && "Entity does not have this component");
    auto* array = get_componentArray<T>();
    return array ? array->get_component(entity) : nullptr;
  }

  template <typename T>
  const T* World::get_component(EntityID entity) const
  {
    assert(hasComponent<T>(entity) && "Entity does not have this component");
    auto* array = const_cast<World*>(this)->get_componentArray<T>();
    return array ? array->get_component(entity) : nullptr;
  }

  template <typename T>
  bool World::hasComponent(EntityID entity) const
  {
    ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
    auto it = entitySignatures.find(entity);
    return it != entitySignatures.end() && it->second.test(typeId);
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
    for (EntityID entity : activeEntities)
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
    for (EntityID entityId : activeEntities)
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
    for (EntityID entityId : activeEntities)
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
    auto* array = const_cast<World*>(this)->get_componentArray<T>();
    return array ? array->size() : 0;
  }

} // namespace ECS
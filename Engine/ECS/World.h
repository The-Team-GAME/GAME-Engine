#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "System.h"
#include "EventManager.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <cassert>

namespace ECS
{

  // Main ECS World
  class World
  {
  private:
    EntityID nextEntityId = 1;
    std::unordered_set<EntityID> activeEntities;
    std::unordered_set<EntityID> entitiesToDestroy;

    // Entity signatures for fast component checking
    std::unordered_map<EntityID, ComponentSignature> entitySignatures;

    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> componentArrays;
    std::vector<std::unique_ptr<System>> systems;
    std::vector<Event> eventQueue;

    template <typename T>
    ComponentArray<T>* getComponentArray()
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

  public:
    // Component registration
    template <typename T>
    void registerComponent()
    {
      ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
      assert(componentArrays.find(typeId) == componentArrays.end() &&
        "Component already registered");
      componentArrays[typeId] = std::make_unique<ComponentArray<T>>();
    }

    // Entity management
    Entity createEntity();
    Entity getEntity(EntityID id);
    Entity cloneEntity(EntityID sourceId);
    void destroyEntity(EntityID id);

    template <typename... T>
    void destroyEntitiesWith()
    {
      for (EntityID entity : activeEntities)
      {
        if (hasComponents<T...>(entity))
        {
          destroyEntity(entity);
        }
      }
    }

    bool isEntityActive(EntityID id) const;

    // Component management
    template <typename T, typename... Args>
    void addComponent(EntityID entity, Args &&...args)
    {
      assert(!hasComponent<T>(entity) && "Entity already has this component");
      ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
      auto* array = getComponentArray<T>();
      // The assert is no longer strictly necessary as getComponentArray now ensures creation.
      // assert(array && "Component not registered");

      array->addComponent(entity, T{ std::forward<Args>(args)... });
      entitySignatures[entity].set(typeId);
      entityComponentChanged(entity);
    }

    template <typename T>
    void removeComponent(EntityID entity)
    {
      assert(hasComponent<T>(entity) && "Entity does not have this component");
      ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
      auto* array = getComponentArray<T>();
      if (array)
      {
        array->removeComponent(entity);
        entitySignatures[entity].reset(typeId);
        entityComponentChanged(entity);
      }
    }

    template <typename T>
    T* getComponent(EntityID entity)
    {
      assert(hasComponent<T>(entity) && "Entity does not have this component");
      auto* array = getComponentArray<T>();
      return array ? array->getComponent(entity) : nullptr;
    }

    template <typename T>
    const T* getComponent(EntityID entity) const
    {
      assert(hasComponent<T>(entity) && "Entity does not have this component");
      auto* array = const_cast<World*>(this)->getComponentArray<T>();
      return array ? array->getComponent(entity) : nullptr;
    }

    template <typename T>
    bool hasComponent(EntityID entity) const
    {
      ComponentTypeID typeId = ComponentTypeManager::getTypeId<T>();
      auto it = entitySignatures.find(entity);
      return it != entitySignatures.end() && it->second.test(typeId);
    }

    template <typename... T>
    bool hasComponents(EntityID entity) const
    {
      return (hasComponent<T>(entity) && ...);
    }

    // System management
    template <typename T, typename... Args>
    T* addSystem(Args &&...args)
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
    void removeSystem()
    {
      systems.erase(
        std::remove_if(systems.begin(), systems.end(),
          [](const std::unique_ptr<System>& system)
          {
            return dynamic_cast<T*>(system.get()) != nullptr;
          }),
        systems.end());
    }

    // Event management
    template <typename T, typename... Args>
    void sendEvent(Args&&... args)
    {
      eventQueue.emplace_back(T{ std::forward<Args>(args)... });
    }

    // Main update loop
    void update();

    // Main draw loop
    void draw();

    // Query methods - return Entity instead of EntityID
    template <typename... T>
    std::vector<Entity> getEntitiesWith() const
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
    Entity getFirstEntityWith() const
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

    std::vector<Entity> getAllEntities() const;
    size_t getEntityCount() const;

    template <typename T>
    size_t getComponentCount() const
    {
      auto* array = const_cast<World*>(this)->getComponentArray<T>();
      return array ? array->size() : 0;
    }

  private:
    void entityComponentChanged(EntityID entity);
    bool systemMatchesEntity(const System& system, EntityID entity) const;
  };

} // namespace ECS
#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "System.h"
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
    Entity createEntity()
    {
      EntityID id = nextEntityId++;
      activeEntities.insert(id);
      entitySignatures[id] = ComponentSignature{}; // Initialize empty signature
      return Entity(id, this);
    }

    Entity getEntity(EntityID id)
    {
      if (activeEntities.find(id) != activeEntities.end())
      {
        return Entity(id, this);
      }
      return Entity(INVALID_ENTITY, this);
    }

    Entity cloneEntity(EntityID sourceId)
    {
      if (!isEntityActive(sourceId))
      {
        return Entity(INVALID_ENTITY, this); // Source entity doesn't exist or is inactive
      }

      Entity newEntity = createEntity();

      // Copy signature from source to new entity
      const auto& sourceSignature = entitySignatures.at(sourceId);
      entitySignatures[newEntity.getId()] = sourceSignature;

      // Copy all components using the signature
      for (auto const& [typeId, componentArray] : componentArrays)
      {
        if (sourceSignature.test(typeId))
        {
          componentArray->cloneComponent(sourceId, newEntity.getId());
        }
      }

      // Notify systems about the new entity
      entityComponentChanged(newEntity.getId());

      return newEntity;
    }

    void destroyEntity(EntityID id)
    {
      entitiesToDestroy.insert(id);
    }

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

    bool isEntityActive(EntityID id) const
    {
      return activeEntities.find(id) != activeEntities.end();
    }

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

    // Main update loop
    void update()
    {
      // Update all systems
      for (auto& system : systems)
      {
        system->update();
      }

      // Clean up destroyed entities
      for (EntityID entityId : entitiesToDestroy)
      {
        if (activeEntities.erase(entityId))
        {
          // Remove from all component arrays
          for (auto& [typeId, array] : componentArrays)
          {
            array->removeEntity(entityId);
          }

          // Remove signature
          entitySignatures.erase(entityId);

          // Remove from all systems
          for (auto& system : systems)
          {
            system->removeEntity(entityId);
          }
        }
      }
      entitiesToDestroy.clear();
    }

    // Query methods
    template <typename... T>
    std::vector<EntityID> getEntitiesWith() const
    {
      std::vector<EntityID> result;
      for (EntityID entity : activeEntities)
      {
        if (hasComponents<T...>(entity))
        {
          result.push_back(entity);
        }
      }
      return result;
    }

    template <typename... T>
    EntityID getFirstEntityWith() const
    {
      for (EntityID entity : activeEntities)
      {
        if (hasComponents<T...>(entity))
        {
          return entity;
        }
      }
      return INVALID_ENTITY;
    }

    std::vector<EntityID> getAllEntities() const
    {
      return std::vector<EntityID>(activeEntities.begin(), activeEntities.end());
    }

    size_t getEntityCount() const
    {
      return activeEntities.size();
    }

    template <typename T>
    size_t getComponentCount() const
    {
      auto* array = const_cast<World*>(this)->getComponentArray<T>();
      return array ? array->size() : 0;
    }

  private:
    void entityComponentChanged(EntityID entity)
    {
      for (auto& system : systems)
      {
        if (systemMatchesEntity(*system, entity))
        {
          system->addEntity(entity);
        }
        else
        {
          system->removeEntity(entity);
        }
      }
    }

    bool systemMatchesEntity(const System& system, EntityID entity) const
    {
      if (!isEntityActive(entity))
        return false;

      auto it = entitySignatures.find(entity);
      if (it == entitySignatures.end())
        return false;

      const auto& requiredSignature = system.getRequiredSignature();
      const auto& entitySignature = it->second;

      // Fast bitwise check: entity has all required components
      return (requiredSignature & entitySignature) == requiredSignature;
    }
  };

} // namespace ECS
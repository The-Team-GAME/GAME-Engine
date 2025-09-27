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
    ComponentArray<T>* getComponentArray();

  public:
    // Component registration
    template <typename T>
    void registerComponent();

    // Entity management
    Entity createEntity();
    Entity getEntity(EntityID id);
    Entity cloneEntity(EntityID sourceId);
    void destroyEntity(EntityID id);

    template <typename... T>
    void destroyEntitiesWith();

    bool isEntityActive(EntityID id) const;

    // Component management
    template <typename T, typename... Args>
    void addComponent(EntityID entity, Args &&...args);

    template <typename T>
    void removeComponent(EntityID entity);

    template <typename T>
    T* getComponent(EntityID entity);

    template <typename T>
    const T* getComponent(EntityID entity) const;

    template <typename T>
    bool hasComponent(EntityID entity) const;

    template <typename... T>
    bool hasComponents(EntityID entity) const;

    // System management
    template <typename T, typename... Args>
    T* addSystem(Args &&...args);

    template <typename T>
    void removeSystem();

    // Event management
    template <typename T, typename... Args>
    void sendEvent(Args&&... args);

    // Main update loop
    void update();

    // Main draw loop
    void draw();

    // Query methods - return Entity instead of EntityID
    template <typename... T>
    std::vector<Entity> getEntitiesWith() const;

    template <typename... T>
    Entity getFirstEntityWith() const;

    std::vector<Entity> getAllEntities() const;
    size_t getEntityCount() const;

    template <typename T>
    size_t getComponentCount() const;

  private:
    void entityComponentChanged(EntityID entity);
    bool systemMatchesEntity(const System& system, EntityID entity) const;
  };

} // namespace ECS


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
#include <functional>

namespace ECS
{

  // Main ECS World
  class World
  {
  private:
    EntityID next_entity_id = 1;
    std::unordered_set<EntityID> active_entities;
    std::unordered_set<EntityID> entities_to_destory;

    // Entity signatures for fast component checking
    std::unordered_map<EntityID, ComponentSignature> entity_signatures;

    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
    std::vector<std::unique_ptr<System>> systems;
    std::vector<Event> eventQueue;

    // Direct event handlers - map from event type to list of handlers
    std::unordered_map<EventTypeID, std::vector<std::function<void(const Event&)>>> eventHandlers;

    template <typename T>
    ComponentArray<T>* get_component_array();

  public:
    // Component registration
    template <typename T>
    void register_component();

    // Entity management
    Entity create_entity();
    Entity get_entity(EntityID id);
    Entity clone_entity(EntityID source_id);
    void destory_entity(EntityID id);

    template <typename... T>
    void destroyEntitiesWith();

    bool isEntityActive(EntityID id) const;

    // Component management
    template <typename T, typename... Args>
    void addComponent(EntityID entity, Args &&...args);

    template <typename T>
    void removeComponent(EntityID entity);

    template <typename T>
    T* get_component(EntityID entity);

    template <typename T>
    const T* get_component(EntityID entity) const;

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

    // Direct event handling without systems
    template <typename T>
    void addEventListener(std::function<void(const T&)> handler);

    template <typename T>
    void addEventListener(std::function<void(const Event&)> handler);

    void removeEventListener(EventTypeID eventType);

    template <typename T>
    void removeEventListener();

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
    size_t get_componentCount() const;

  private:
    void entityComponentChanged(EntityID entity);
    bool systemMatchesEntity(const System& system, EntityID entity) const;
  };

} // namespace ECS


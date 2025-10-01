#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include "EventManager.h"
#include <unordered_set>

namespace ECS {

  class World; // Forward declaration
  class Entity; // Forward declaration

  // System base class with clean required components specification
  class System {
  protected:
    World* world = nullptr;
    std::unordered_set<EntityID> entities;
    ComponentSignature requiredSignature;
    EventSignature eventSignature;

  public:
    virtual ~System();

    // Clean way to specify required components!
    template<typename... Components>
    void require_components();

    // Clean way to specify listened-to events!
    template<typename... Events>
    void listen_to_events();

    // System logic
    virtual void update();

    // Drawing/rendering logic
    virtual void draw();

    // Override for handling a specific event for a specific entity
    virtual void onEvent(const Event& event, EntityID entity);

    // Override for per-entity logic
    virtual void updateEntity(Entity& entity);

    // Override for per-entity drawing logic (const reference because rendering shouldn't modify entities)
    virtual void draw_entity(const Entity& entity);

    // Override for entity lifecycle events
    virtual void onEntityAdded(const Entity& entity);
    virtual void onEntityRemoved(const Entity& entity);

    // Internal methods
    void addEntity(EntityID entity);
    void removeEntity(EntityID entity);

    const std::unordered_set<EntityID>& getEntities() const;
    const ComponentSignature& getRequiredSignature() const;
    const EventSignature& getEventSignature() const;

  private:
    friend class World;
    void setWorld(World* w);
  };

} // namespace ECS


#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include "EventManager.h"
#include <unordered_set>
#include "Entity.h"

namespace ECS {

  class World; // Forward declaration

  // System base class with clean required components specification
  class System {
  protected:
    World* world = nullptr;
    std::unordered_set<EntityID> entities;
    ComponentSignature requiredSignature;
    EventSignature eventSignature;

  public:
    virtual ~System() = default;

    // Clean way to specify required components!
    template<typename... Components>
    void requireComponents() {
      requiredSignature.reset();
      ((requiredSignature.set(ComponentTypeManager::getTypeId<Components>())), ...);
    }

    // Clean way to specify listened-to events!
    template<typename... Events>
    void listenToEvents() {
      eventSignature.reset();
      ((eventSignature.set(EventTypeManager::getTypeId<Events>())), ...);
    }

    // System logic
    void update() {
      for (EntityID entityId : entities) {
        Entity entity(entityId, world);
        updateEntity(entity);
      }
    }

    // Drawing/rendering logic
    void draw() {
      for (EntityID entityId : entities) {
        Entity entity(entityId, world);
        drawEntity(entity);
      }
    }

    // Override for handling a specific event for a specific entity
    virtual void onEvent([[maybe_unused]] const Event& event, [[maybe_unused]] EntityID entity) {}

    // Override for per-entity logic
    virtual void updateEntity([[maybe_unused]] Entity& entity) {}

    // Override for per-entity drawing logic (const reference because rendering shouldn't modify entities)
    virtual void drawEntity([[maybe_unused]] const Entity& entity) {}

    // Override for entity lifecycle events
    virtual void onEntityAdded([[maybe_unused]] const Entity& entity) {}
    virtual void onEntityRemoved([[maybe_unused]] const Entity& entity) {}

    // Internal methods
    void addEntity(EntityID entity) {
      if (entities.insert(entity).second) {
        onEntityAdded(entity);
      }
    }

    void removeEntity(EntityID entity) {
      if (entities.erase(entity)) {
        onEntityRemoved(entity);
      }
    }

    const std::unordered_set<EntityID>& getEntities() const { return entities; }
    const ComponentSignature& getRequiredSignature() const { return requiredSignature; }
    const EventSignature& getEventSignature() const { return eventSignature; }

  private:
    friend class World;
    void setWorld(World* w) { world = w; }
  };

} // namespace ECS

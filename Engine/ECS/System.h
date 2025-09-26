#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include "EventManager.h"
#include <unordered_set>

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

    // Override for system logic
    virtual void update() {
      for (EntityID entityId : entities) {
        updateEntity(entityId);
      }
    }

    // Override for drawing/rendering logic
    virtual void draw() {
      for (EntityID entityId : entities) {
        drawEntity(entityId);
      }
    }

    // Override for handling a specific event for a specific entity
    virtual void onEvent([[maybe_unused]] const Event& event, [[maybe_unused]] EntityID entity) {}

    // Override for per-entity logic
    virtual void updateEntity([[maybe_unused]] EntityID entity) {}

    // Override for per-entity drawing logic
    virtual void drawEntity([[maybe_unused]] EntityID entity) {}

    // Override for entity lifecycle events
    virtual void onEntityAdded([[maybe_unused]] EntityID entity) {}
    virtual void onEntityRemoved([[maybe_unused]] EntityID entity) {}

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

#pragma once

#include "Types.h"
#include "ComponentManager.h"
#include <unordered_set>

namespace ECS {

  class World; // Forward declaration

  // System base class with clean required components specification
  class System {
  protected:
    World* world = nullptr;
    std::unordered_set<EntityID> entities;
    ComponentSignature requiredSignature;

  public:
    virtual ~System() = default;

    // Clean way to specify required components!
    template<typename... Components>
    void requireComponents() {
      requiredSignature.reset();
      ((requiredSignature.set(ComponentTypeManager::getTypeId<Components>())), ...);
    }

    // Override for system logic
    virtual void update(float deltaTime) {
      for (EntityID entityId : entities) {
        updateEntity(entityId, deltaTime);
      }
    }

    // Override for per-entity logic
    virtual void updateEntity([[maybe_unused]] EntityID entity, [[maybe_unused]] float deltaTime) {}

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

  private:
    friend class World;
    void setWorld(World* w) { world = w; }
  };

} // namespace ECS

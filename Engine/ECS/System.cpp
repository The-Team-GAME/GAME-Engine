#include "System.h"
#include "Entity.h"
#include "World.h"

namespace ECS {

  System::~System() = default;

  void System::update() {
    // Create Entity handles for each entity ID
    for (EntityID entityId : entities) {
      Entity entity(entityId, world);
      updateEntity(entity);
    }
  }

  void System::draw() {
    // Create Entity handles for each entity ID
    for (EntityID entityId : entities) {
      const Entity entity(entityId, world);
      drawEntity(entity);
    }
  }

  void System::onEvent([[maybe_unused]] const Event& event, [[maybe_unused]] EntityID entity) {}

  void System::updateEntity([[maybe_unused]] Entity& entity) {}

  void System::drawEntity([[maybe_unused]] const Entity& entity) {}

  void System::onEntityAdded([[maybe_unused]] const Entity& entity) {}

  void System::onEntityRemoved([[maybe_unused]] const Entity& entity) {}

  void System::addEntity(EntityID entity) {
    if (entities.insert(entity).second) {
      Entity entityHandle(entity, world);
      onEntityAdded(entityHandle);
    }
  }

  void System::removeEntity(EntityID entity) {
    if (entities.erase(entity) > 0) {
      Entity entityHandle(entity, world);
      onEntityRemoved(entityHandle);
    }
  }

  const std::unordered_set<EntityID>& System::getEntities() const {
    return entities;
  }

  const ComponentSignature& System::getRequiredSignature() const {
    return requiredSignature;
  }

  const EventSignature& System::getEventSignature() const {
    return eventSignature;
  }

  void System::setWorld(World* w) {
    world = w;
  }

} // namespace ECS
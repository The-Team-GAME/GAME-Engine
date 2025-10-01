#include "Entity.h"
#include "World.h"

namespace ECS {

  Entity::Entity(EntityID id, World* world) : id(id), world(world) {}

  EntityID Entity::getId() const {
    return id;
  }

  bool Entity::isValid() const {
    return id != INVALID_ENTITY;
  }

  void Entity::destroy() {
    world->destroy_entity(id);
  }

  bool Entity::operator==(const Entity& other) const {
    return id == other.id;
  }

  bool Entity::operator!=(const Entity& other) const {
    return id != other.id;
  }

} // namespace ECS
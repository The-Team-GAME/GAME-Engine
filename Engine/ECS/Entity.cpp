#pragma once

#include "Entity.h"
#include "World.h"

namespace ECS {

  void Entity::destroy() {
    world->destroyEntity(id);
  }

} // namespace ECS
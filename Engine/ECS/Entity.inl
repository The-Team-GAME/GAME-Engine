#pragma once
#include "Entity.h"
#include "World.h"

namespace ECS {

  template<typename T, typename... Args>
  Entity& Entity::addComponent(Args&&... args) {
    world->addComponent<T>(id, std::forward<Args>(args)...);
    return *this;
  }

  template<typename T>
  Entity& Entity::removeComponent() {
    world->removeComponent<T>(id);
    return *this;
  }

  template<typename T>
  T* Entity::getComponent() {
    return world->getComponent<T>(id);
  }

  template<typename T>
  const T* Entity::getComponent() const {
    return world->getComponent<T>(id);
  }

  template<typename T>
  bool Entity::hasComponent() const {
    return world->hasComponent<T>(id);
  }

  template<typename... T>
  bool Entity::hasComponents() const {
    return world->hasComponents<T...>(id);
  }

} // namespace ECS
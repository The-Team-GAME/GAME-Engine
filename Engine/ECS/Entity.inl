#pragma once
#include "Entity.h"
#include "World.h" // Add this include

namespace ECS {

  template<typename T, typename... Args>
  Entity& Entity::add_component(Args&&... args) {
    world->add_component<T>(id, std::forward<Args>(args)...);
    return *this;
  }

  template<typename T>
  Entity& Entity::remove_component() {
    world->remove_component<T>(id);
    return *this;
  }

  template<typename T>
  T* Entity::get_component() {
    return world->get_component<T>(id);
  }

  template<typename T>
  const T* Entity::get_component() const {
    return world->get_component<T>(id);
  }

  template<typename T>
  bool Entity::has_component() const {
    return world->has_component<T>(id);
  }

  template<typename... T>
  bool Entity::has_components() const {
    return world->has_components<T...>(id);
  }

} // namespace ECS
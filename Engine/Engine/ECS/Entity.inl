//-----------------------------------------------------------------------------------
// file:    Entity.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for the Entity class.
//          Provides inline implementations for component manipulation operations
//          that forward calls to the World namespace.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Entity.h"
#include "World.h"

namespace ECS {

  template<typename T, typename... Args>
  Entity& Entity::add_component(Args&&... args) {
    World::add_component<T>(id, std::forward<Args>(args)...);
    return *this;
  }

  template<typename T>
  Entity& Entity::remove_component() {
    World::remove_component<T>(id);
    return *this;
  }

  template<typename T>
  T* Entity::get_component() {
    return World::get_component<T>(id);
  }

  template<typename T>
  const T* Entity::get_component() const {
    return World::get_component<T>(id);
  }

  template<typename T>
  bool Entity::has_component() const {
    return World::has_component<T>(id);
  }

  template<typename... T>
  bool Entity::has_components() const {
    return World::has_components<T...>(id);
  }

} // namespace ECS
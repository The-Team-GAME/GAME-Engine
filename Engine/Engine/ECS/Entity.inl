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

namespace Engine {
  namespace ECS {

    template<typename T, typename... Args>
    Entity& Entity::add_component(Args&&... args) {
      ECS::add_component<T>(*this, std::forward<Args>(args)...);
      return *this;
    }

    template<typename T>
    Entity& Entity::remove_component() {
      ECS::remove_component<T>(*this);
      return *this;
    }

    template<typename T>
    T* Entity::get_component() {
      return ECS::get_component<T>(*this);
    }

    template<typename T>
    const T* Entity::get_component() const {
      return ECS::get_component<T>(*this);
    }

    template<typename T>
    bool Entity::has_component() const {
      return ECS::has_component<T>(*this);
    }

    template<typename... T>
    bool Entity::has_components() const {
      return ECS::has_components<T...>(*this);
    }

  } // namespace ECS
} // namespace Engine
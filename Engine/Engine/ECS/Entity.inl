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
      requires std::is_constructible_v<T, Args...>
    Entity Entity::add_component(Args&&... args) {
      ECS::add_component<T>(*this, std::forward<Args>(args)...);
      return *this;
    }

    template<typename T>
    Entity Entity::remove_component() {
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

    // ============================================================================
    // Layer Management Template Implementation
    // ============================================================================

    template<LayerID... Layers>
    Entity Entity::add_to_layers() {
      ECS::add_to_layers<Layers...>(*this);
      return *this;
    }

    template<LayerID... Layers>
    Entity Entity::remove_from_layers() {
      ECS::remove_from_layers<Layers...>(*this);
      return *this;
    }

    template<LayerID... Layers>
    Entity Entity::set_layers() {
      ECS::set_layers<Layers...>(*this);
      return *this;
    }

    template<LayerID... Layers>
    bool Entity::is_on_any_layer() const {
      return ECS::is_on_any_layer<Layers...>(*this);
    }

    template<LayerID... Layers>
    bool Entity::is_on_all_layers() const {
      return ECS::is_on_all_layers<Layers...>(*this);
    }

  } // namespace ECS
} // namespace Engine
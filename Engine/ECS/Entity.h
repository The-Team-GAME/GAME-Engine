//-----------------------------------------------------------------------------------
// file:    Entity.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Entity class which provides a clean handle-based
//          API for working with entities in the ECS. Entity acts as a lightweight
//          wrapper around an entity ID with convenient component access methods.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Types.h"

namespace ECS {

  /* ============================ //
  // Entity handle for clean API  //
  // ============================ */
  class Entity {
  private:
    EntityID id;

  public:
    Entity(EntityID id = INVALID_ENTITY);

    EntityID get_id() const;
    bool is_valid() const;

    template<typename T, typename... Args>
    Entity& add_component(Args&&... args);

    template<typename T>
    Entity& remove_component();

    template<typename T>
    T* get_component();

    template<typename T>
    const T* get_component() const;

    template<typename T>
    bool has_component() const;

    template<typename... T>
    bool has_components() const;

    void destroy();

    bool operator==(const Entity& other) const;
    bool operator!=(const Entity& other) const;
  };

} // namespace ECS


//-----------------------------------------------------------------------------------
// file:    HierarchyComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for entity hierarchy system (not yet implemented).
//          Defines Parent, Children, Root, and Sibling components for future
//          implementation of parent-child relationships between entities.
//          Will enable scene graph functionality and hierarchical transformations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "../ECS/ECS.h"
#include "../ECS/Macros.h"

namespace Components
{
  struct Parent {
    Engine::ECS::EntityID parent_id;  // Stored as ID for proper serialization
    Math::Vec2D local_offset;
    Math::Vec2D prev_parent_pos; // Track parent's previous position for delta calculation
    bool offset_initialized = false; // If false, HierarchySystem will auto-calculate on first update

    Parent() : parent_id(Engine::ECS::INVALID_ENTITY) {}

    // Auto-calculate offset constructor (most common usage)
    Parent(Engine::ECS::EntityID parent)
      : parent_id(parent), local_offset(0.0f, 0.0f), prev_parent_pos(0.0f, 0.0f), offset_initialized(false) {
    }

    // Manual offset constructor (for advanced usage)
    Parent(Engine::ECS::EntityID parent, Math::Vec2D offset)
      : parent_id(parent), local_offset(offset), prev_parent_pos(0.0f, 0.0f), offset_initialized(true) {
    }
  };

  struct Children {
    std::vector<Engine::ECS::EntityID> child_ids;  // Stored as IDs for proper serialization
  };

  struct Root {}; // Mark root entities (no parent)
}

REGISTER_COMPONENT(Components::Parent, parent_id, local_offset, prev_parent_pos, offset_initialized)
REGISTER_COMPONENT(Components::Children, child_ids)
REGISTER_COMPONENT(Components::Root)
//-----------------------------------------------------------------------------------
// file:    HierarchySystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that manages entity parent-child relationships and propagates
//          transforms from parents to children. Entities with a Parent component
//          will have their position updated relative to their parent's position
//          plus their local offset.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../MonkeyEngine.h"
#include "../Components/HierarchyComponents.h"

namespace Engine {
  namespace Systems
  {
    class HierarchySystem : public Engine::ECS::System {
    public:
      HierarchySystem() {
        require_components<Components::Parent, Components::Transform2D>();
      }

      void update_entity(Engine::ECS::Entity& entity) override {
        auto* parent_comp = entity.get_component<Components::Parent>();
        auto* child_transform = entity.get_component<Components::Transform2D>();
        
        if (!parent_comp || !child_transform) return;

        // Get parent entity
        auto parent = parent_comp->parent_id;
        
        // Check if parent still exists
        if (!parent.is_valid()) {
          // Parent was destroyed, remove parent component
          entity.remove_component<Components::Parent>();
          return;
        }

        // Get parent's transform
        auto* parent_transform = parent.get_component<Components::Transform2D>();
        if (!parent_transform) {
          // Parent has no transform, can't propagate
          return;
        }

        // Auto-calculate local offset on first update (if not initialized)
        if (!parent_comp->offset_initialized) {
          parent_comp->local_offset = child_transform->position - parent_transform->position;
          parent_comp->prev_parent_pos = parent_transform->position;
          parent_comp->offset_initialized = true;
        }

        // Calculate parent's movement delta
        Math::Vec2D parent_delta = parent_transform->position - parent_comp->prev_parent_pos;

        // Apply delta to child
        if (parent_delta.get_magnitude_sq() > 0.0001f) {
          child_transform->position += parent_delta;
        }

        // Update previous parent position for next frame
        parent_comp->prev_parent_pos = parent_transform->position;
        
        // Update local offset to reflect new relative position (in case child moved independently)
        parent_comp->local_offset = child_transform->position - parent_transform->position;
      }
    };
  }
}

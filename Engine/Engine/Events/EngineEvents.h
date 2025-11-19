//-----------------------------------------------------------------------------------
// file:    EngineEvents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines engine-level event structures. Includes entity
//          selection events for synchronizing selection state across systems
//          and UI panels, and entity modification events for undo/redo support.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../TypeDef.h"
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include <functional>
#include <string>

namespace Engine {
  namespace Events {

    /* ======================= //
    // Entity selection events //
    // ======================= */
    struct SelectEntityEvent {
      Engine::ECS::EntityID entity_id;
      bool start_dragging = false; // If true, start dragging immediately
    };

    /* ============================ //
    // Entity modification events   //
    // ============================ */

    // Event to modify an entity's transform with undo/redo support
    struct ModifyEntityTransformEvent {
      Engine::ECS::EntityID entity_id;
      Components::Transform2D new_transform;
      Components::Transform2D old_transform;
      bool record_undo = true; // Set to false to skip undo recording (for undo/redo itself)
    };

    // Event to delete an entity with undo/redo support
    struct DeleteEntityEvent {
      Engine::ECS::EntityID entity_id;
      bool record_undo = true; // Set to false to skip undo recording (for undo/redo itself)
    };

    // Event to add a component to an entity with undo/redo support
    template<typename ComponentType>
    struct AddComponentEvent {
      Engine::ECS::EntityID entity_id;
      ComponentType component_data;
      bool record_undo = true;
    };

    // Event to remove a component from an entity with undo/redo support
    struct RemoveComponentEvent {
      Engine::ECS::EntityID entity_id;
      Engine::ECS::ComponentTypeID component_type_id;
      bool record_undo = true;
    };

    // Event to modify a component's data with undo/redo support
    template<typename ComponentType>
    struct ModifyComponentEvent {
      Engine::ECS::EntityID entity_id;
      ComponentType new_data;
      ComponentType old_data;
      bool record_undo = true;
    };

  } // namespace Events
} // namespace Engine

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
#include "../Reflection/ReflectionLibrary.h"
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

      SelectEntityEvent(Engine::ECS::EntityID id, bool drag = false)
        : entity_id(id), start_dragging(drag) {
      }
    };

    /* ============================ //
    // Entity modification events   //
    // ============================ */

    // Event to delete an entity with undo/redo support
    struct DeleteEntityEvent {
      Engine::ECS::EntityID entity_id;
      bool record_undo = true; // Set to false to skip undo recording (for undo/redo itself)
    };

    // Event to add a component to an entity with undo/redo support (generic, reflection-based)
    struct AddComponentEvent {
      Engine::ECS::EntityID entity_id;
      Engine::ECS::ComponentTypeID component_type_id;
      ReflectionLibrary::GenericValue component_data;
      bool record_undo = true;
    };

    // Event to remove a component from an entity with undo/redo support
    struct RemoveComponentEvent {
      Engine::ECS::EntityID entity_id;
      Engine::ECS::ComponentTypeID component_type_id;
      bool record_undo = true;
    };

    // Event to modify a component's data with undo/redo support (generic, reflection-based)
    struct ModifyComponentEvent {
      Engine::ECS::EntityID entity_id;
      Engine::ECS::ComponentTypeID component_type_id;
      ReflectionLibrary::GenericValue new_data;
      ReflectionLibrary::GenericValue old_data;
      bool record_undo = true;
    };

    /* ============================ //
    // Toast notification events    //
    // ============================ */
    enum class ToastType {
      Info,
      Warning,
      Error
    };

    struct ToastEvent {
      std::string message;
      ToastType type;
      float duration;

      ToastEvent(const std::string& msg, ToastType t = ToastType::Info, float dur = 3.0f)
        : message(msg), type(t), duration(dur) {
      }
    };

  } // namespace Events
} // namespace Engine

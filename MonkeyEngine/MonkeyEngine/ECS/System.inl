//-----------------------------------------------------------------------------------
// file:    System.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for the System class.
//          Provides inline implementations for specifying required components
//          and event subscriptions using variadic templates.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "System.h"
#include "ComponentManager.h"
#include "../EventManager/EventManager.h"

namespace Engine {
  namespace ECS {

    template<typename... Components>
    void System::require_components() {
      required_signature.reset();
      ((required_signature.set(ComponentTypeManager::get_type_id<Components>())), ...);
    }

    template<typename... Components>
    void System::exclude_components() {
      excluded_signature.reset();
      ((excluded_signature.set(ComponentTypeManager::get_type_id<Components>())), ...);
    }

    template<typename... Events>
    void System::listen_to_events() {
      event_signature.reset();
      ((event_signature.set(EventManager::EventTypeManager::get_type_id<Events>())), ...);
    }

    template<LayerID... Layers>
    void System::process_layers() {
      active_layers.reset();
      ((active_layers.set(Layers)), ...);
    }

  } // namespace ECS
} // namespace Engine
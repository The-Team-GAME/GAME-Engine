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
#include "EventManager.h"

namespace ECS {

  template<typename... Components>
  void System::require_components() {
    requiredSignature.reset();
    ((requiredSignature.set(ComponentTypeManager::getTypeId<Components>())), ...);
  }

  template<typename... Events>
  void System::listen_to_events() {
    eventSignature.reset();
    ((eventSignature.set(EventTypeManager::getTypeId<Events>())), ...);
  }

} // namespace ECS
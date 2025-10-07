//-----------------------------------------------------------------------------------
// file:    ComponentManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the ComponentTypeManager which automatically generates
//          unique type IDs for component types at compile time. Uses template
//          specialization to ensure each component type gets a unique ID.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Types.h"

namespace ECS {

  /* ===================================== //
  // Automatic component type ID generation //
  // ===================================== */
  class ComponentTypeManager {
  private:
    static ComponentTypeID next_type_id;

  public:
    template<typename T>
    static ComponentTypeID get_type_id();

    static ComponentTypeID get_next_available_id() {
      return next_type_id + 1;
    }
  };

} // namespace ECS


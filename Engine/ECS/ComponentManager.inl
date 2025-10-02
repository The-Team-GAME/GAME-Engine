//-----------------------------------------------------------------------------------
// file:    ComponentManager.inl
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the template methods for ComponentTypeManager.
//          Uses static variables inside template functions to generate unique IDs
//          for each component type.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "ComponentManager.h"

namespace ECS {

  template<typename T>
  ComponentTypeID ComponentTypeManager::getTypeId() {
    static ComponentTypeID typeId = ++nextTypeId;
    return typeId;
  }

} // namespace ECS
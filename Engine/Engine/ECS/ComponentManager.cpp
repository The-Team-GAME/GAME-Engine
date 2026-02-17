//-----------------------------------------------------------------------------------
// file:    ComponentManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the ComponentTypeManager static member initialization.
//          Initializes the counter used for automatic component type ID generation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ComponentManager.h"

namespace Engine {
  namespace ECS {
    ComponentTypeID ComponentTypeManager::next_type_id = 0;

    std::unordered_map<std::type_index, ComponentTypeID>& ComponentTypeManager::get_type_map() {
      static std::unordered_map<std::type_index, ComponentTypeID> map;
      return map;
    }
  }
}
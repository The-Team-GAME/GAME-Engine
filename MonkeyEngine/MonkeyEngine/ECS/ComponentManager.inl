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
#include <typeinfo>

namespace Engine {
  namespace ECS {

    template<typename T>
    ComponentTypeID ComponentTypeManager::get_type_id() {
      // Cache locally per-instantiation for performance.
      // The actual ID comes from the shared type map, ensuring
      // cross-module consistency (same typeid(T) -> same ID).
      // Uses typeid(T).name() (a string) as key instead of type_index
      // so that the map entries survive DLL unload/reload cycles.
      static ComponentTypeID type_id = []() {
        auto& map = get_type_map();
        std::string key = typeid(T).name();
        auto it = map.find(key);
        if (it != map.end()) return it->second;
        ComponentTypeID id = ++next_type_id;
        map[key] = id;
        return id;
        }();
      return type_id;
    }

  } // namespace ECS
} // namespace Engine
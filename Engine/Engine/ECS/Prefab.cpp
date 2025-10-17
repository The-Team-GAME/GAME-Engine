//-----------------------------------------------------------------------------------
// file:    Prefab.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the Prefab system for creating reusable entity
//          templates. Provides functions for creating and instantiating prefabs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Prefab.h"
#include "World.h"

namespace Engine {
  namespace ECS {

    Prefab::Prefab(const std::string& name, std::function<void(Entity&)> setup)
      : name(name), setup_function(std::move(setup)) {
    }

    Prefab create_prefab(const std::string& name, std::function<void(Entity&)> setup) {
      return Prefab(name, std::move(setup));
    }

    Entity instantiate_prefab(const Prefab& prefab) {
      Entity entity = create_entity();
      if (prefab.setup_function) {
        prefab.setup_function(entity);
      }
      return entity;
    }

  } // namespace ECS
} // namespace Engine
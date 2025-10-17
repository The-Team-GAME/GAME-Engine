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
//          templates. Includes the Prefab class and PrefabManager for registration
//          and instantiation of prefabs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Prefab.h"
#include "World.h"
#include <cassert>

namespace Engine {
  namespace ECS {

    /* ====================== //
    // Prefab Implementation  //
    // ====================== */
    Prefab::Prefab(const std::string& name, std::function<void(Entity&)> setup)
      : name(name), setup_function(std::move(setup)) {}

    Entity Prefab::instantiate() const {
      Entity entity = create_entity();
      if (setup_function) {
        setup_function(entity);
      }
      return entity;
    }

    const std::string& Prefab::get_name() const {
      return name;
    }

    /* ================================== //
    // Prefab Manager Implementation      //
    // ================================== */
    namespace PrefabManager {
      namespace Internal {
        std::unordered_map<std::string, std::unique_ptr<Prefab>> prefabs;
      }

      void register_prefab(const std::string& name, std::function<void(Entity&)> setup) {
        assert(Internal::prefabs.find(name) == Internal::prefabs.end() && 
               "Prefab with this name already exists");
        Internal::prefabs[name] = std::make_unique<Prefab>(name, std::move(setup));
      }

      Entity instantiate_prefab(const std::string& name) {
        auto it = Internal::prefabs.find(name);
        assert(it != Internal::prefabs.end() && "Prefab not found");
        return it->second->instantiate();
      }

      bool has_prefab(const std::string& name) {
        return Internal::prefabs.find(name) != Internal::prefabs.end();
      }

      void clear_prefabs() {
        Internal::prefabs.clear();
      }
    }

  } // namespace ECS
} // namespace Engine
//-----------------------------------------------------------------------------------
// file:    Prefab.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Prefab system for the ECS. Prefabs are reusable
//          entity templates that store component configurations. They can be
//          instantiated multiple times to create entities with the same setup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "Entity.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

namespace Engine {
  namespace ECS {

    /* ====================================== //
    // Prefab - Reusable entity template      //
    // ====================================== */
    class Prefab {
    private:
      std::string name;
      std::function<void(Entity&)> setup_function;

    public:
      Prefab(const std::string& name, std::function<void(Entity&)> setup);

      Entity instantiate() const;
      const std::string& get_name() const;
    };

    /* ====================================== //
    // Prefab Manager - Registry for prefabs  //
    // ====================================== */
    namespace PrefabManager {
      void register_prefab(const std::string& name, std::function<void(Entity&)> setup);
      Entity instantiate_prefab(const std::string& name);
      bool has_prefab(const std::string& name);
      void clear_prefabs();
      
      namespace Internal {
        extern std::unordered_map<std::string, std::unique_ptr<Prefab>> prefabs;
      }
    }

  } // namespace ECS
} // namespace Engine
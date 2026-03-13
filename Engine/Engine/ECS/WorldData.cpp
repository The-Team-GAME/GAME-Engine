//-----------------------------------------------------------------------------------
// file:    WorldData.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the centralized container accessor for the ECS World.
//          Uses a static local variable to ensure single instance initialization.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "WorldData.h"
#include <unordered_map>

namespace Engine {
  namespace ECS {
    namespace Internal {

      WorldContainers* g_containers = nullptr;
      std::vector<std::unique_ptr<System>> g_systems;

      // Construct-on-first-use to avoid static initialization order fiasco
      static std::unordered_map<WorldID, WorldContainers*>& worlds() {
        static std::unordered_map<WorldID, WorldContainers*> instance;
        return instance;
      }
      static WorldID& next_world_id() {
        static WorldID instance = 1;
        return instance;
      }
      static WorldID& active_world_id() {
        static WorldID instance = INVALID_WORLD;
        return instance;
      }

      WorldContainers& init_containers() {
        g_containers = new WorldContainers();
        // Register the initial world
        WorldID id = next_world_id()++;
        g_containers->world_id = id;
        worlds()[id] = g_containers;
        active_world_id() = id;
        return *g_containers;
      }

      WorldID get_active_world_id() {
        return active_world_id();
      }

      std::unordered_map<WorldID, WorldContainers*>& get_world_registry() {
        return worlds();
      }

      WorldID get_next_world_id() {
        return next_world_id()++;
      }

    } // namespace Internal
  } // namespace ECS
} // namespace Engine
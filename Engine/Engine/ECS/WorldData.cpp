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
//          Uses a heap-allocated object so we can control destruction timing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "WorldData.h"
#include "../Logger.h"

namespace Engine {
  namespace ECS {
    namespace Internal {

      // Use a pointer so we can control destruction timing
      // This avoids false positive memory leak reports from CRT
      static WorldContainers* s_containers = nullptr;

      WorldContainers& get_containers() {
        if (!s_containers) {
          // Use operator new directly to avoid going through MemoryManager
          // (which may not be initialized yet during static init)
          s_containers = new WorldContainers();
          Log::memory("WorldContainers created at {}", static_cast<void*>(s_containers));
        }
        return *s_containers;
      }

      void cleanup_containers() {
        Log::memory("cleanup_containers() called, s_containers = {}", static_cast<void*>(s_containers));
        if (!s_containers) {
          Log::memory("cleanup_containers() - already null, returning");
          return;
        }

        WorldContainers& containers = *s_containers;

        // Clear all containers to properly destruct contained objects
        // This MUST be called before MemoryManager::shutdown()
        containers.collision_callbacks.clear();
        containers.component_arrays.clear();
        containers.systems.clear();
        containers.components_to_remove.clear();
        containers.entity_signatures.clear();
        containers.entity_layers.clear();
        containers.prefab_instances.clear();
        containers.layer_name_to_id.clear();
        containers.layer_id_to_name.clear();
        containers.layer_entities.clear();
        containers.active_entities.clear();
        containers.entities_to_destroy.clear();

        Log::memory("cleanup_containers() - all containers cleared, deleting WorldContainers");

        // Now destroy the container object itself
        // This will deallocate the internal HashMap/Array bucket arrays
        delete s_containers;
        s_containers = nullptr;

        Log::memory("cleanup_containers() - WorldContainers deleted successfully");
      }

    } // namespace Internal
  } // namespace ECS
} // namespace Engine
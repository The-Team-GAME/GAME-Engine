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

namespace Engine {
  namespace ECS {
    namespace Internal {

      WorldContainers& get_containers() {
        static WorldContainers containers;
        return containers;
      }

      void cleanup_containers() {
        WorldContainers& containers = get_containers();
        
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
      }

    } // namespace Internal
  } // namespace ECS
} // namespace Engine
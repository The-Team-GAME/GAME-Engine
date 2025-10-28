//-----------------------------------------------------------------------------------
// file:    WorldData.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the internal data containers for the ECS World.
//          Provides centralized access to all ECS state through a container struct.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "ComponentArray.h"
#include "System.h"
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Engine {
  namespace ECS {
    namespace Internal {

      struct WorldContainers {
        EntityID next_entity_id = 1;
        std::unordered_set<EntityID> active_entities;
        std::unordered_set<EntityID> entities_to_destroy;
        std::unordered_map<EntityID, ComponentSignature> entity_signatures;
        std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
        std::vector<std::unique_ptr<System>> systems;
      };

      /*=================================================================================
      Function Brief:
      Returns a reference to the global ECS container struct holding all world state.

      Parameters:
      None.

      Where to call it:
      Internally by the ECS when accessing entity, component, or system data.
      ==================================================================================*/
      WorldContainers& get_containers();

    } // namespace Internal
  } // namespace ECS
} // namespace Engine
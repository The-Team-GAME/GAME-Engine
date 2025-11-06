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
#include "SparseSet.h"
#include "ComponentArray.h"
#include "System.h"
#include <unordered_map>
#include <memory>
#include <vector>

namespace Engine {
  namespace ECS {
    namespace Internal {

      struct WorldContainers {
        EntityID next_entity_id = 1;
        LayerID next_layer_id = 2; // Start at 2 (0 = INVALID_LAYER, 1 = DEFAULT_LAYER)
        SparseSet<EntityID> active_entities;
        SparseSet<EntityID> entities_to_destroy;
        std::unordered_map<EntityID, ComponentSignature> entity_signatures;
        std::unordered_map<EntityID, LayerSignature> entity_layers;
        std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
        std::vector<std::unique_ptr<System>> systems;
        std::vector<std::pair<EntityID, ComponentTypeID>> components_to_remove;
        std::unordered_map<std::string, LayerID> layer_name_to_id;
        std::vector<std::string> layer_id_to_name;
        std::vector<SparseSet<EntityID>> layer_entities;
        std::bitset<MAX_LAYERS> layer_active_state; // Track which layers are active/enabled

        // Constructor to initialize default layer
        WorldContainers() {
          // Pre-register default layer as ID 1 (DEFAULT_LAYER constant)
          layer_name_to_id["default"] = DEFAULT_LAYER;
          layer_id_to_name.resize(2); // Resize to hold ID 0 (unused) and ID 1 (default)
          layer_id_to_name[DEFAULT_LAYER] = "default";
          layer_entities.resize(2); // Resize to hold layer 0 (unused) and layer 1 (default)
          layer_active_state.set(); // All layers are active by default
        }
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
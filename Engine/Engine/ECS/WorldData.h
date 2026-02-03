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
//          Now uses custom Memory Manager containers (HashMap, Array) instead of
//          STL containers for entity/component storage. This ensures all ECS memory
//          flows through the pool allocators for efficient allocation.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "SparseSet.h"
#include "ComponentArray.h"
#include "System.h"
#include "../Memory/Memory.h"
#include <memory>
#include <array>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace Engine {
  namespace ECS {
    class Entity; // Forward declaration

    namespace Internal {

      /* ====================================== //
      // PrefabInstanceData - Tracks prefab     //
      // instances and field overrides          //
      // ====================================== */
      struct PrefabInstanceData {
        std::string prefab_id; // Now stores UUID instead of file path
        // Map: ComponentTypeID -> set of overridden field paths (e.g., "color", "scale.x")
        std::unordered_map<ComponentTypeID, std::unordered_set<std::string>> field_overrides;
      };

      struct WorldContainers {
        EntityID next_entity_id = 1;
        LayerID next_layer_id = 2; // Start at 2 (0 = INVALID_LAYER, 1 = DEFAULT_LAYER)
        SparseSet<EntityID> active_entities;
        SparseSet<EntityID> entities_to_destroy;
        // Using Memory::HashMap for entity signatures - maps EntityID to ComponentSignature
        Memory::HashMap<EntityID, ComponentSignature> entity_signatures;
        Memory::HashMap<EntityID, LayerSignature> entity_layers;
        Memory::HashMap<EntityID, PrefabInstanceData> prefab_instances;
        // Component arrays still use unique_ptr for polymorphism
        Memory::HashMap<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays;
        // Systems use unique_ptr for polymorphism
        Memory::Array<std::unique_ptr<System>> systems;
        Memory::Array<std::pair<EntityID, ComponentTypeID>> components_to_remove;
        // Layer management uses Memory containers
        Memory::HashMap<std::string, LayerID> layer_name_to_id;
        Memory::Array<std::string> layer_id_to_name;
        Memory::Array<SparseSet<EntityID>> layer_entities;
        std::bitset<MAX_LAYERS> layer_active_state; // Track which layers are active/enabled

        // Collision matrix: collision_matrix[layerA][layerB] = true if they should collide
        // Matrix is symmetric: collision_matrix[A][B] == collision_matrix[B][A]
        std::array<std::bitset<MAX_LAYERS>, MAX_LAYERS> collision_matrix;

        // Collision callbacks: map from layer pair key to callback function
        // Key is created by make_layer_pair_key(LayerID, LayerID) - asymmetric!
        // (layer_a, layer_b) and (layer_b, layer_a) are different keys
        // u64 key = (layer_a << 32) | layer_b
        // Entity order in callback matches layer order: callback(entity_on_layer_a, entity_on_layer_b)
        Memory::HashMap<u64, std::function<void(Entity, Entity)>> collision_callbacks;

        // One-shot update requests (processed even when paused)
        std::unordered_set<System*> systems_to_update_once;
        std::unordered_set<EntityID> entities_to_update_once;

        // Constructor to initialize default layer
        WorldContainers() {
          // Pre-register default layer as ID 1 (DEFAULT_LAYER constant)
          layer_name_to_id["default"] = DEFAULT_LAYER;
          layer_id_to_name.resize(2); // Resize to hold ID 0 (unused) and ID 1 (default)
          layer_id_to_name[DEFAULT_LAYER] = "default";
          layer_entities.resize(2); // Resize to hold layer 0 (unused) and layer 1 (default)
          layer_active_state.set(); // All layers are active by default

          // Initialize collision matrix: all layers collide with all layers by default
          for (auto& row : collision_matrix) {
            row.set();
          }
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
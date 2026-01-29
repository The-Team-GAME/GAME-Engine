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
//          NOW USES CUSTOM MEMORY MANAGER - No STL containers for core storage.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "ManagedSparseSet.h"
#include "ManagedContainers.h"
#include "ManagedComponentArray.h"
#include "System.h"
#include <bitset>
#include <functional>
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
        std::string prefab_id; // UUID - keeping std::string for file I/O compatibility
        // Map: ComponentTypeID -> set of overridden field paths (e.g., "color", "scale.x")
        ManagedHashMap<ComponentTypeID, ManagedHashSet<std::string>> field_overrides;

        PrefabInstanceData() = default;
        PrefabInstanceData(PrefabInstanceData&&) = default;
        PrefabInstanceData& operator=(PrefabInstanceData&&) = default;
      };

      struct WorldContainers {
        EntityID next_entity_id = 1;
        LayerID next_layer_id = 2; // Start at 2 (0 = INVALID_LAYER, 1 = DEFAULT_LAYER)

        // Entity tracking - using ManagedSparseSet (custom memory)
        ManagedSparseSet<EntityID> active_entities;
        ManagedSparseSet<EntityID> entities_to_destroy;

        // Entity data - using ManagedHashMap (custom memory)
        ManagedHashMap<EntityID, ComponentSignature> entity_signatures;
        ManagedHashMap<EntityID, LayerSignature> entity_layers;
        ManagedHashMap<EntityID, PrefabInstanceData> prefab_instances;

        // Component storage - using ManagedHashMap + ManagedUniquePtr (custom memory)
        ManagedHashMap<ComponentTypeID, ManagedUniquePtr<IManagedComponentArray>> component_arrays;

        // Systems - using ManagedArray + ManagedUniquePtr (custom memory)
        ManagedArray<ManagedUniquePtr<System>> systems;

        // Deferred component removal
        struct ComponentRemoval {
          EntityID entity;
          ComponentTypeID type;
        };
        ManagedArray<ComponentRemoval> components_to_remove;

        // Layer management - using ManagedHashMap and ManagedArray (custom memory)
        ManagedHashMap<std::string, LayerID> layer_name_to_id;
        ManagedArray<std::string> layer_id_to_name;
        ManagedArray<ManagedSparseSet<EntityID>> layer_entities;
        std::bitset<MAX_LAYERS> layer_active_state; // Track which layers are active/enabled

        // Collision matrix: collision_matrix[layerA][layerB] = true if they should collide
        // Matrix is symmetric: collision_matrix[A][B] == collision_matrix[B][A]
        std::bitset<MAX_LAYERS> collision_matrix[MAX_LAYERS];

        // Collision callbacks: map from layer pair key to callback function
        // Key is created by make_layer_pair_key(LayerID, LayerID) - asymmetric!
        // u64 key = (layer_a << 32) | layer_b
        ManagedHashMap<u64, std::function<void(Entity, Entity)>> collision_callbacks;

        // Constructor to initialize default layer
        WorldContainers() {
          // Pre-register default layer as ID 1 (DEFAULT_LAYER constant)
          layer_name_to_id.insert(std::string("default"), DEFAULT_LAYER);
          layer_id_to_name.resize(2); // Resize to hold ID 0 (unused) and ID 1 (default)
          layer_id_to_name[DEFAULT_LAYER] = "default";
          layer_entities.resize(2); // Resize to hold layer 0 (unused) and layer 1 (default)
          layer_active_state.set(); // All layers are active by default

          // Initialize collision matrix: all layers collide with all layers by default
          for (size_t i = 0; i < MAX_LAYERS; ++i) {
            collision_matrix[i].set();
          }
        }

        // Disable copy (managed containers don't support copy)
        WorldContainers(const WorldContainers&) = delete;
        WorldContainers& operator=(const WorldContainers&) = delete;
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
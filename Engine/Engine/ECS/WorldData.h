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
#include "../EngineExport.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <unordered_set>

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
        WorldID world_id = INVALID_WORLD;
        EntityID next_entity_id = 1;
        LayerID next_layer_id = 2; // Start at 2 (0 = INVALID_LAYER, 1 = DEFAULT_LAYER)
        SparseSet<EntityID> active_entities;
        SparseSet<EntityID> entities_to_destroy;
        SparseSet<EntityID> deactivated_entities; // Entities suspended (kept alive but not iterated/rendered)
        std::vector<ComponentSignature> entity_signatures;  // Indexed by EntityID — O(1) direct access
        std::vector<LayerID> entity_highest_layer; // Cached highest layer per entity (indexed by EntityID)
        std::vector<LayerSignature> entity_layers;  // Indexed by EntityID — O(1) direct access
        std::unordered_map<EntityID, std::string> entity_names;
        std::unordered_map<EntityID, PrefabInstanceData> prefab_instances;
        std::array<std::unique_ptr<IComponentArray>, MAX_COMPONENTS> component_arrays{};
        std::vector<std::pair<EntityID, ComponentTypeID>> components_to_remove;
        std::unordered_map<std::string, LayerID> layer_name_to_id;
        std::vector<std::string> layer_id_to_name;
        std::vector<SparseSet<EntityID>> layer_entities;
        std::bitset<MAX_LAYERS> layer_active_state; // Track which layers are active/enabled

        // Collision matrix: collision_matrix[layerA][layerB] = true if they should collide
        // Matrix is symmetric: collision_matrix[A][B] == collision_matrix[B][A]
        std::array<std::bitset<MAX_LAYERS>, MAX_LAYERS> collision_matrix;

        // Collision callbacks: map from layer pair key to callback function
        // Key is created by make_layer_pair_key(LayerID, LayerID) - asymmetric!
        // (layer_a, layer_b) and (layer_b, layer_a) are different keys
        // u64 key = (layer_a << 32) | layer_b
        // Entity order in callback matches layer order: callback(entity_on_layer_a, entity_on_layer_b)
        std::unordered_map<u64, std::function<void(Entity, Entity)>> collision_callbacks;

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

      // Active world pointer (points to the currently active WorldContainers)
      ENGINE_API extern WorldContainers* g_containers;

      // Global systems vector (shared across all worlds)
      ENGINE_API extern std::vector<std::unique_ptr<System>> g_systems;

      ENGINE_API WorldContainers& init_containers();
      inline WorldContainers& get_containers() {
        if (g_containers) [[likely]] return *g_containers;
        return init_containers();
      }

      // Accessor for global systems
      inline std::vector<std::unique_ptr<System>>& get_systems() { return g_systems; }

      // World registry helpers (implemented in WorldData.cpp)
      ENGINE_API WorldID get_active_world_id();
      ENGINE_API std::unordered_map<WorldID, WorldContainers*>& get_world_registry();
      ENGINE_API WorldID get_next_world_id();

    } // namespace Internal
  } // namespace ECS
} // namespace Engine
//-----------------------------------------------------------------------------------
// file:    CollisionCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the collision callback system for layer-based
//          collision events. Stores and manages callbacks in WorldData.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "CollisionCallbacks.h"
#include "World.h"
#include "WorldData.h"

namespace Engine {
  namespace ECS {
    namespace CollisionCallbacks {

      // Helper function to create a unique key for a layer pair
      // Uses a symmetric key so (A,B) and (B,A) map to the same callback
      static u64 make_layer_pair_key(LayerID layer_a, LayerID layer_b) {
        LayerID min_layer = layer_a < layer_b ? layer_a : layer_b;
        LayerID max_layer = layer_a < layer_b ? layer_b : layer_a;
        return (static_cast<u64>(min_layer) << 32) | static_cast<u64>(max_layer);
      }

      void register_callback(LayerID layer_a, LayerID layer_b, CollisionCallback callback) {
        auto& containers = Internal::get_containers();
        u64 key = make_layer_pair_key(layer_a, layer_b);
        containers.collision_callbacks[key] = std::move(callback);
      }

      void register_callback(const std::string& layer_a_name, const std::string& layer_b_name, CollisionCallback callback) {
        LayerID layer_a = get_layer_id(layer_a_name);
        LayerID layer_b = get_layer_id(layer_b_name);

        if (layer_a != INVALID_LAYER && layer_b != INVALID_LAYER) {
          register_callback(layer_a, layer_b, std::move(callback));
        }
      }

      void unregister_callback(LayerID layer_a, LayerID layer_b) {
        auto& containers = Internal::get_containers();
        u64 key = make_layer_pair_key(layer_a, layer_b);
        containers.collision_callbacks.erase(key);
      }

      void unregister_callback(const std::string& layer_a_name, const std::string& layer_b_name) {
        LayerID layer_a = get_layer_id(layer_a_name);
        LayerID layer_b = get_layer_id(layer_b_name);

        if (layer_a != INVALID_LAYER && layer_b != INVALID_LAYER) {
          unregister_callback(layer_a, layer_b);
        }
      }

      void clear_all_callbacks() {
        auto& containers = Internal::get_containers();
        containers.collision_callbacks.clear();
      }

      void invoke_callbacks(Entity entity_a, Entity entity_b) {
        auto& containers = Internal::get_containers();

        // Get layers for both entities
        LayerSignature layers_a = entity_a.get_layers();
        LayerSignature layers_b = entity_b.get_layers();

        // Check all layer pairs between the two entities
        for (LayerID layer_a = 0; layer_a < MAX_LAYERS; ++layer_a) {
          if (!layers_a.test(layer_a)) continue;

          for (LayerID layer_b = 0; layer_b < MAX_LAYERS; ++layer_b) {
            if (!layers_b.test(layer_b)) continue;

            u64 key = make_layer_pair_key(layer_a, layer_b);
            auto it = containers.collision_callbacks.find(key);
            if (it != containers.collision_callbacks.end()) {
              // Invoke callback - entity order matches layer order
              if (layer_a <= layer_b) {
                it->second(entity_a, entity_b);
              }
              else {
                it->second(entity_b, entity_a);
              }
            }
          }
        }
      }

      bool has_callback(LayerID layer_a, LayerID layer_b) {
        auto& containers = Internal::get_containers();
        u64 key = make_layer_pair_key(layer_a, layer_b);
        return containers.collision_callbacks.find(key) != containers.collision_callbacks.end();
      }

    } // namespace CollisionCallbacks
  } // namespace ECS
} // namespace Engine

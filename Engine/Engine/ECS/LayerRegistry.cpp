//-----------------------------------------------------------------------------------
// file:    LayerRegistry.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LayerRegistry collision matrix management.
//          Provides methods for configuring and querying layer collision relationships.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "LayerRegistry.h"
#include "World.h"

namespace Engine {
  namespace ECS {

    LayerRegistry::LayerRegistry() {
      // By default, all layers collide with all layers
      reset_to_default();
    }

    void LayerRegistry::enable_collision(LayerID layer_a, LayerID layer_b) {
      if (layer_a < MAX_LAYERS && layer_b < MAX_LAYERS) {
        collision_matrix[layer_a].set(layer_b);
        collision_matrix[layer_b].set(layer_a); // Symmetric
      }
    }

    void LayerRegistry::enable_collision(const std::string& layer_a_name, const std::string& layer_b_name) {
      LayerID layer_a = get_layer_id(layer_a_name);
      LayerID layer_b = get_layer_id(layer_b_name);

      if (layer_a != INVALID_LAYER && layer_b != INVALID_LAYER) {
        enable_collision(layer_a, layer_b);
      }
    }

    void LayerRegistry::disable_collision(LayerID layer_a, LayerID layer_b) {
      if (layer_a < MAX_LAYERS && layer_b < MAX_LAYERS) {
        collision_matrix[layer_a].reset(layer_b);
        collision_matrix[layer_b].reset(layer_a); // Symmetric
      }
    }

    void LayerRegistry::disable_collision(const std::string& layer_a_name, const std::string& layer_b_name) {
      LayerID layer_a = get_layer_id(layer_a_name);
      LayerID layer_b = get_layer_id(layer_b_name);

      if (layer_a != INVALID_LAYER && layer_b != INVALID_LAYER) {
        disable_collision(layer_a, layer_b);
      }
    }

    bool LayerRegistry::should_collide(LayerID layer_a, LayerID layer_b) const {
      if (layer_a >= MAX_LAYERS || layer_b >= MAX_LAYERS) {
        return false;
      }
      return collision_matrix[layer_a].test(layer_b);
    }

    bool LayerRegistry::should_collide(const LayerSignature& signature_a, const LayerSignature& signature_b) const {
      // Check if any layer in signature_a collides with any layer in signature_b
      for (LayerID layer_a = 0; layer_a < MAX_LAYERS; ++layer_a) {
        if (!signature_a.test(layer_a)) continue;

        for (LayerID layer_b = 0; layer_b < MAX_LAYERS; ++layer_b) {
          if (!signature_b.test(layer_b)) continue;

          if (should_collide(layer_a, layer_b)) {
            return true;
          }
        }
      }
      return false;
    }

    void LayerRegistry::set_layer_collisions(LayerID layer, const std::bitset<MAX_LAYERS>& collides_with) {
      if (layer < MAX_LAYERS) {
        collision_matrix[layer] = collides_with;

        // Ensure symmetry: update all other layers that should collide with this one
        for (LayerID other_layer = 0; other_layer < MAX_LAYERS; ++other_layer) {
          if (collides_with.test(other_layer)) {
            collision_matrix[other_layer].set(layer);
          }
          else {
            collision_matrix[other_layer].reset(layer);
          }
        }
      }
    }

    std::bitset<MAX_LAYERS> LayerRegistry::get_layer_collisions(LayerID layer) const {
      if (layer < MAX_LAYERS) {
        return collision_matrix[layer];
      }
      return std::bitset<MAX_LAYERS>();
    }

    void LayerRegistry::reset_to_default() {
      // By default, all layers collide with all layers
      for (auto& row : collision_matrix) {
        row.set();
      }
    }

    void LayerRegistry::clear_all() {
      // Clear all collisions
      for (auto& row : collision_matrix) {
        row.reset();
      }
    }

  } // namespace ECS
} // namespace Engine

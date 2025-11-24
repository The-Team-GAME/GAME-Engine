//-----------------------------------------------------------------------------------
// file:    LayerRegistry.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the LayerRegistry namespace which provides functions
//          for managing the collision matrix that determines which layers should
//          collide with which other layers. The collision matrix data is stored
//          in WorldData.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include <string>
#include <bitset>
#include <functional>

namespace Engine {
  namespace ECS {
    class Entity; // Forward declaration

    namespace LayerRegistry {

      // Callback signature: void callback(Entity entityA, Entity entityB)
      // entityA is on layerA, entityB is on layerB (order matches registration)
      using CollisionCallback = std::function<void(Entity, Entity)>;

      /*=================================================================================
      Function Brief:
      Enables collision between two layers. Order doesn't matter as the matrix is
      symmetric.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Where to call it:
      During initialization or when dynamically configuring collision relationships.
      ==================================================================================*/
      void enable_layer_collision(LayerID layer_a, LayerID layer_b);

      /*=================================================================================
      Function Brief:
      Enables collision between two layers using layer names.

      Parameters:
      - layer_a_name: First layer name
      - layer_b_name: Second layer name

      Where to call it:
      During initialization or when dynamically configuring collision relationships.
      ==================================================================================*/
      void enable_layer_collision(const std::string& layer_a_name, const std::string& layer_b_name);

      /*=================================================================================
      Function Brief:
      Disables collision between two layers. Order doesn't matter as the matrix is
      symmetric.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Where to call it:
      When dynamically changing collision relationships.
      ==================================================================================*/
      void disable_layer_collision(LayerID layer_a, LayerID layer_b);

      /*=================================================================================
      Function Brief:
      Disables collision between two layers using layer names.

      Parameters:
      - layer_a_name: First layer name
      - layer_b_name: Second layer name

      Where to call it:
      When dynamically changing collision relationships.
      ==================================================================================*/
      void disable_layer_collision(const std::string& layer_a_name, const std::string& layer_b_name);

      /*=================================================================================
      Function Brief:
      Checks if two layers should collide according to the collision matrix.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Returns:
      True if the layers should collide, false otherwise.

      Where to call it:
      In collision detection to filter collisions based on layers.
      ==================================================================================*/
      bool should_layers_collide(LayerID layer_a, LayerID layer_b);

      /*=================================================================================
      Function Brief:
      Checks if any layer in signature_a should collide with any layer in signature_b.

      Parameters:
      - signature_a: First entity's layer signature
      - signature_b: Second entity's layer signature

      Returns:
      True if any layer pair should collide, false otherwise.

      Where to call it:
      In collision detection when checking if two entities should collide.
      ==================================================================================*/
      bool should_layers_collide(const LayerSignature& signature_a, const LayerSignature& signature_b);

      /*=================================================================================
      Function Brief:
      Sets an entire row of the collision matrix for a specific layer.

      Parameters:
      - layer: The layer ID to configure
      - collides_with: Bitset indicating which layers this layer should collide with

      Where to call it:
      When bulk-configuring collision relationships for a layer.
      ==================================================================================*/
      void set_layer_collisions(LayerID layer, const std::bitset<MAX_LAYERS>& collides_with);

      /*=================================================================================
      Function Brief:
      Gets all layers that a specific layer collides with.

      Parameters:
      - layer: The layer ID to query

      Returns:
      Bitset indicating which layers this layer collides with.

      Where to call it:
      When querying collision configuration or for debugging.
      ==================================================================================*/
      std::bitset<MAX_LAYERS> get_layer_collisions(LayerID layer);

      /*=================================================================================
      Function Brief:
      Resets the collision matrix to default state (all layers collide with all layers).

      Where to call it:
      During initialization or when resetting collision configuration.
      ==================================================================================*/
      void reset_to_default();

      /*=================================================================================
      Function Brief:
      Clears all collision relationships (no layers collide).

      Where to call it:
      When starting fresh with collision configuration.
      ==================================================================================*/
      void clear_all();

      /*=================================================================================
      Function Brief:
      Registers a callback function to be called when entities on two specific layers
      collide. The callback receives both entities involved in the collision.

      IMPORTANT: Callbacks are asymmetric! The entity order in the callback matches
      the layer order during registration. If you register (layer_a, layer_b), the
      callback will receive (entity_on_layer_a, entity_on_layer_b). To handle both
      directions, register both (A, B) and (B, A) with appropriate callbacks.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID
      - callback: Function to call when collision occurs (receives entities in order)

      Example:
      // Player projectile hits enemy - order matters!
      register_collision_callback(player_projectile_layer, enemy_layer, [](Entity projectile, Entity enemy) {
        // projectile is first, enemy is second
        if (auto* health = enemy.get_component<Health>()) {
          health->take_damage(10);
        }
        projectile.destroy();
      });

      // If you want enemy hitting projectile to have different logic:
      register_collision_callback(enemy_layer, player_projectile_layer, [](Entity enemy, Entity projectile) {
        // enemy is first, projectile is second
        // Different handling if needed
      });

      Where to call it:
      During initialization when setting up gameplay collision logic.
      ==================================================================================*/
      void register_collision_callback(LayerID layer_a, LayerID layer_b, CollisionCallback callback);

      /*=================================================================================
      Function Brief:
      Registers a callback using layer names instead of IDs.

      IMPORTANT: Callbacks are asymmetric! The entity order in the callback matches
      the layer order during registration.

      Parameters:
      - layer_a_name: First layer name
      - layer_b_name: Second layer name
      - callback: Function to call when collision occurs (receives entities in order)

      Where to call it:
      During initialization when setting up gameplay collision logic.
      ==================================================================================*/
      void register_collision_callback(const std::string& layer_a_name, const std::string& layer_b_name, CollisionCallback callback);

      /*=================================================================================
      Function Brief:
      Unregisters the callback for a specific layer pair.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Where to call it:
      When you want to remove collision logic for a layer pair.
      ==================================================================================*/
      void unregister_collision_callback(LayerID layer_a, LayerID layer_b);

      /*=================================================================================
      Function Brief:
      Unregisters the callback using layer names.

      Parameters:
      - layer_a_name: First layer name
      - layer_b_name: Second layer name

      Where to call it:
      When you want to remove collision logic for a layer pair.
      ==================================================================================*/
      void unregister_collision_callback(const std::string& layer_a_name, const std::string& layer_b_name);

      /*=================================================================================
      Function Brief:
      Clears all registered collision callbacks.

      Where to call it:
      When resetting gameplay systems or cleaning up.
      ==================================================================================*/
      void clear_all_collision_callbacks();

      /*=================================================================================
      Function Brief:
      Invokes any registered callbacks for the layer pairs involved in a collision.
      This is called automatically by the collision system.

      Parameters:
      - entity_a: First entity involved in collision
      - entity_b: Second entity involved in collision

      Where to call it:
      Internally by CollisionSystem when a collision is detected.
      ==================================================================================*/
      void invoke_collision_callbacks(Entity entity_a, Entity entity_b);

      /*=================================================================================
      Function Brief:
      Checks if a callback is registered for a specific layer pair.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Returns:
      True if a callback exists for this layer pair, false otherwise.

      Where to call it:
      For debugging or conditional callback registration.
      ==================================================================================*/
      bool has_collision_callback(LayerID layer_a, LayerID layer_b);

    } // namespace LayerRegistry
  } // namespace ECS
} // namespace Engine

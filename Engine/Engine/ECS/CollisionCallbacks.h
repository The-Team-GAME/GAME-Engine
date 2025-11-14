//-----------------------------------------------------------------------------------
// file:    CollisionCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the collision callback system for layer-based collision
//          events. Allows registering custom logic that executes when entities on
//          specific layers collide (e.g., player projectile hitting enemy).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Types.h"
#include "Entity.h"
#include <functional>
#include <unordered_map>
#include <utility>

namespace Engine {
  namespace ECS {
    namespace CollisionCallbacks {

      // Callback signature: void callback(Entity entityA, Entity entityB)
      // entityA is on layerA, entityB is on layerB
      using CollisionCallback = std::function<void(Entity, Entity)>;

      /*=================================================================================
      Function Brief:
      Registers a callback function to be called when entities on two specific layers
      collide. The callback receives both entities involved in the collision.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID
      - callback: Function to call when collision occurs

      Example:
      register_callback("player_projectile", "enemy", [](Entity projectile, Entity enemy) {
        // Damage enemy
        if (auto* health = enemy.get_component<Health>()) {
          health->take_damage(10);
        }
        // Destroy projectile
        projectile.destroy();
      });

      Where to call it:
      During initialization when setting up gameplay collision logic.
      ==================================================================================*/
      void register_callback(LayerID layer_a, LayerID layer_b, CollisionCallback callback);

      /*=================================================================================
      Function Brief:
      Registers a callback using layer names instead of IDs.

      Parameters:
      - layer_a_name: First layer name
      - layer_b_name: Second layer name
      - callback: Function to call when collision occurs

      Where to call it:
      During initialization when setting up gameplay collision logic.
      ==================================================================================*/
      void register_callback(const std::string& layer_a_name, const std::string& layer_b_name, CollisionCallback callback);

      /*=================================================================================
      Function Brief:
      Unregisters the callback for a specific layer pair.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Where to call it:
      When you want to remove collision logic for a layer pair.
      ==================================================================================*/
      void unregister_callback(LayerID layer_a, LayerID layer_b);

      /*=================================================================================
      Function Brief:
      Unregisters the callback using layer names.

      Parameters:
      - layer_a_name: First layer name
      - layer_b_name: Second layer name

      Where to call it:
      When you want to remove collision logic for a layer pair.
      ==================================================================================*/
      void unregister_callback(const std::string& layer_a_name, const std::string& layer_b_name);

      /*=================================================================================
      Function Brief:
      Clears all registered collision callbacks.

      Where to call it:
      When resetting the game state or switching levels.
      ==================================================================================*/
      void clear_all_callbacks();

      /*=================================================================================
      Function Brief:
      Invokes the appropriate callback for a collision between two entities, if one
      is registered for their layers.

      Parameters:
      - entity_a: First entity in collision
      - entity_b: Second entity in collision

      Where to call it:
      Called by the collision system when a collision is detected.
      ==================================================================================*/
      void invoke_callbacks(Entity entity_a, Entity entity_b);

      /*=================================================================================
      Function Brief:
      Checks if a callback is registered for a specific layer pair.

      Parameters:
      - layer_a: First layer ID
      - layer_b: Second layer ID

      Returns:
      True if a callback exists for this layer pair, false otherwise.

      Where to call it:
      For debugging or conditional logic based on callback existence.
      ==================================================================================*/
      bool has_callback(LayerID layer_a, LayerID layer_b);

    } // namespace CollisionCallbacks
  } // namespace ECS
} // namespace Engine

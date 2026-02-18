//-----------------------------------------------------------------------------------
// file:    InteractableSystem.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   System that handles proximity-based interactions between players and
//          interactable entities. Checks if player is within the interaction radius
//          and invokes on_enter, on_update, and on_exit callbacks accordingly.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "../Components/InteractableComponents.h"

namespace Engine
{
  namespace Systems
  {
    /// System that processes Interactable components and triggers callbacks
    /// based on player proximity. Requires entities to have both Interactable
    /// and Transform2D components.
    class InteractableSystem : public Engine::ECS::System
    {
    public:
      InteractableSystem() {
        require_components<Components::Interactable, Components::Transform2D>();
      }

      void update_entity(Engine::ECS::Entity entity) override {
        if (Engine::Time::get_paused()) return;

        auto* interactable = entity.get_component<Components::Interactable>();
        auto* transform = entity.get_component<Components::Transform2D>();

        if (!interactable || !transform) return;

        // Find the closest player entity within range
        Engine::ECS::Entity closest_player;
        f32 closest_distance_sq = std::numeric_limits<f32>::max();
        const f32 radius_sq = interactable->radius * interactable->radius;

        // Query all entities with Player and Transform2D components
        Engine::ECS::for_each_entity_with<Components::Player, Components::Transform2D>(
          [&](Engine::ECS::Entity player_entity) {
            auto* player_transform = player_entity.get_component<Components::Transform2D>();
            if (!player_transform) return;

            // Calculate distance squared (avoid sqrt for performance)
            f32 dx = player_transform->position.x - transform->position.x;
            f32 dy = player_transform->position.y - transform->position.y;
            f32 distance_sq = dx * dx + dy * dy;

            if (distance_sq < closest_distance_sq) {
              closest_distance_sq = distance_sq;
              closest_player = player_entity;
            }
          }
        );

        bool player_now_in_range = closest_player.is_valid() && closest_distance_sq <= radius_sq;

        // State transition: Player entered range
        if (player_now_in_range && !interactable->player_in_range) {
          interactable->player_in_range = true;
          interactable->last_player_id = closest_player.get_id();

          // Call on_enter callback: void(Entity interactable, Entity player)
          if (!interactable->on_enter.empty()) {
            auto enter_func = interactable->on_enter.get<void(Engine::ECS::Entity, Engine::ECS::Entity)>();
            if (enter_func) {
              enter_func(entity, closest_player);
            }
          }
        }
        // State: Player still in range
        else if (player_now_in_range && interactable->player_in_range) {
          // Update last player ID in case a different player is now closer
          interactable->last_player_id = closest_player.get_id();

          // Call on_update callback: void(Entity interactable, Entity player)
          if (!interactable->on_update.empty()) {
            auto update_func = interactable->on_update.get<void(Engine::ECS::Entity, Engine::ECS::Entity)>();
            if (update_func) {
              update_func(entity, closest_player);
            }
          }
        }
        // State transition: Player exited range
        else if (!player_now_in_range && interactable->player_in_range) {
          // Try to get the last player entity for the exit callback
          Engine::ECS::Entity last_player(interactable->last_player_id);

          // Call on_exit callback: void(Entity interactable, Entity player)
          if (!interactable->on_exit.empty()) {
            auto exit_func = interactable->on_exit.get<void(Engine::ECS::Entity, Engine::ECS::Entity)>();
            if (exit_func) {
              // Pass the last player even if they might no longer be valid
              exit_func(entity, last_player);
            }
          }

          interactable->player_in_range = false;
          interactable->last_player_id = Engine::ECS::INVALID_ENTITY;
        }
      }
    };

  } // namespace Systems
} // namespace Engine

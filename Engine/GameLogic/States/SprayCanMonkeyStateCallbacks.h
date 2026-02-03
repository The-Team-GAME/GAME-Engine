//-----------------------------------------------------------------------------------
// file:    SprayCanMonkeyStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Spray Can Monkey state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"
#include <unordered_map>

namespace SprayCanMonkey
{
  // Helper function to update knockback direction based on player position
  inline void update_contact_knockback_direction(Engine::ECS::Entity entity) {
    auto* attack = entity.get_component<Components::Attack>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* enemy_transform = entity.get_component<Components::Transform2D>();

    if (!attack || !spray_monkey || !enemy_transform) return;

    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) return;

    auto* player_transform = player_entity.get_component<Components::Transform2D>();
    if (!player_transform) return;

    // Calculate direction from enemy to player
    float dir_x = player_transform->position.x - enemy_transform->position.x;
    float sign = (dir_x >= 0.0f ? 1.0f : -1.0f);

    // Set knockback with correct direction
    attack->knockback.x = sign * std::abs(spray_monkey->knockback.x);
    attack->knockback.y = -std::abs(spray_monkey->knockback.y); // Negative for upward
  }

  inline Engine::ECS::Entity spawn_spray_cloud(Engine::ECS::Entity monkey)
  {
    auto* spray = monkey.get_component<Components::SprayCanMonkey>();
    auto* transform = monkey.get_component<Components::Transform2D>();
    auto* flip = monkey.get_component<Components::Flip>();

    if (!spray || !transform) return {};

    float dir = (flip && flip->value.x < 0.f) ? -1.f : 1.f;

    Math::Vec2D cloud_pos = transform->position;
    cloud_pos.x += dir * (spray->spray_range * 0.6f);

    // Use hardcoded values instead of component fields
    const float w = 150.0f;  // was: spray->cloud_width
    const float h = 80.0f;   // was: spray->cloud_height

    Engine::ECS::Entity cloud = Engine::ECS::create_entity()
      .add_component<Components::Tag<"SprayCloud">>()
      .add_component<Components::Attack>(
        5,  // was: spray->cloud_damage_per_tick
        Math::Vec2D(0.f, 0.f),
        true,                        // is_continuous
        0.5f  // was: spray->cloud_damage_interval
      )
      .add_component<Components::Transform2D>(cloud_pos.x, cloud_pos.y, 0.0f, w, h)
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(w, h, false)
      .add_component<Components::Lifetime>(3.0f)  // was: spray->cloud_duration
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"))
      .add_component<Components::Color>(Math::Vec4D(1.f, 1.f, 1.f, 1.f))
      .remove_from_layer("default")
      .add_to_layer("damage_player");

    return cloud;
  }

  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter(Engine::ECS::Entity entity) {
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    if (spray_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      spray_monkey->last_attack_time = Engine::Time::get_time();
    }
    Log::info("Spray Can Monkey: Entered Idle state");
  }

  inline void idle_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();

    if (!spray_monkey) return;

    update_contact_knockback_direction(entity);
    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - spray_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!player_entity.is_valid()) {
      return;
    }

    auto* monkey_transform = entity.get_component<Components::Transform2D>();
    auto* player_transform = player_entity.get_component<Components::Transform2D>();

    if (monkey_transform && player_transform) {
      float dx = player_transform->position.x - monkey_transform->position.x;
      float dy = player_transform->position.y - monkey_transform->position.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      if (distance < spray_monkey->detection_range) {
        state_machine->change_state("Chasing");
      }
    }
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================

  inline void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Spray Can Monkey: Entered Chasing state");
  }

  inline void chasing_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* spray_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();

    if (!spray_monkey) return;


    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    update_contact_knockback_direction(entity);

    if (player_entity.is_valid() && rigid && spray_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - spray_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally, but stop when within attack range
        if (distance > spray_monkey->attack_range) {
          rigid->velocity.x = (dx > 0 ? spray_monkey->chase_speed : -spray_monkey->chase_speed);
          // Update flip direction based on movement
          if (flip) {
            flip->value.x = (dx > 0 ? 1.0f : -1.0f);
          }
          // Don't modify velocity.y - let gravity handle it
        }
        else {
          // Stop moving when close enough to attack
          rigid->velocity.x = 0.0f;
        }

        // Return to idle if player is far away
        if (distance > spray_monkey->chase_range) {
          state_machine->change_state("Idle");
        }

        // If close enough and cooldown has elapsed, go to windup before attacking
        if (distance < spray_monkey->attack_range &&
          (current_time - spray_monkey->last_attack_time) >= spray_monkey->attack_cooldown) {
          state_machine->change_state("Windup");
        }
      }
    }
  }

  inline void chasing_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) rigid->velocity.x = 0.0f;
  }

  // ========================================================================
  // Windup State Callbacks
  // ========================================================================

  inline void windup_on_enter(Engine::ECS::Entity entity) {
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when windup started
    if (spray_monkey) {
      spray_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Spray Can Monkey: Winding up for attack");
  }

  inline void windup_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!spray_monkey) return;

    float current_time = Engine::Time::get_time();
    float windup_elapsed = current_time - spray_monkey->last_attack_time;

    // Get player entity to face them during windup
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid()) {
      auto* spray_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (spray_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - spray_monkey_transform->position.x;
        // Update flip direction to face player during wind-up
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // After windup duration, attack
    if (windup_elapsed >= spray_monkey->attack_windup) {
      spray_monkey->last_attack_time = current_time;
      state_machine->change_state("Spraying");
    }
  }

  // ========================================================================
  // Spraying State Callbacks
  // ========================================================================

  inline void spraying_on_enter(Engine::ECS::Entity entity) {
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();

    if (spray_monkey) {
      spray_monkey->last_attack_time = Engine::Time::get_time();

      spawn_spray_cloud(entity);
    }

    Log::info("Spray Can Monkey: Spraying!");
  }

  inline void spraying_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* animation = entity.get_component<Components::Animation>();

    if (!spray_monkey) return;

    float current_time = Engine::Time::get_time();
    float spray_elapsed = current_time - spray_monkey->last_attack_time;

    // End spray after duration or animation completes
    if (spray_elapsed >= spray_monkey->spray_duration || (animation && animation->has_ended())) {
      state_machine->change_state("Chasing");
    }
  }

  inline void spraying_on_exit(Engine::ECS::Entity entity) {
    // Note: Cloud entity manages its own lifetime
    // No need to manually clean up here
  }

  // ========================================================================
  // Hit State Callbacks
  // ========================================================================

  inline void hit_on_enter(Engine::ECS::Entity entity) {
    Log::info("Spray Can Monkey: Hit!");
  }

  inline void hit_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!spray_monkey) return;

    // When animation ends, detach from hitbox and apply knockback
    if (animation && animation->has_ended() && pending_knockback && rigid) {
      // Detach from hitbox first (stop following player)
      if (entity.has_component<Components::Parent>()) {
        entity.remove_component<Components::Parent>();
      }

      // Apply knockback momentum from Player struct
      rigid->velocity = pending_knockback->knockback;

      // Remove the pending knockback component (one-time use)
      entity.remove_component<Components::PendingKnockback>();
    }

    // Transition to idle only when grounded (after knockback has been applied)
    if (animation && animation->has_ended() && rigid && rigid->on_ground) {
      state_machine->change_state("Idle");
    }
  }

  inline void hit_on_exit(Engine::ECS::Entity entity) {
    // Cleanup: ensure parent is removed (safety check)
    if (entity.has_component<Components::Parent>()) {
      entity.remove_component<Components::Parent>();
    }
  }

  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  inline void death_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Spray Can Monkey: Died!");
  }

  inline void death_on_update(Engine::ECS::Entity entity) {
    auto* animation = entity.get_component<Components::Animation>();
    auto* health = entity.get_component<Components::EnemyHealth>();

    if (health && health->current_hp <= 0) {
      if (animation && animation->has_ended()) {

        Engine::ECS::destroy_entity(entity);
        Log::info("Spray Can Monkey: Destroyed after death animation.");
      }
    }
  }

  inline void death_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    // Should not happen unless respawning
  }

} // namespace SprayCanMonkey

// Register all spray can monkey state callbacks (without reloading)
REGISTER_FUNCTION(spraycan_monkey_idle_enter, void(Engine::ECS::Entity), SprayCanMonkey::idle_on_enter)
REGISTER_FUNCTION(spraycan_monkey_idle_update, void(Engine::ECS::Entity), SprayCanMonkey::idle_on_update)
REGISTER_FUNCTION(spraycan_monkey_chasing_enter, void(Engine::ECS::Entity), SprayCanMonkey::chasing_on_enter)
REGISTER_FUNCTION(spraycan_monkey_chasing_update, void(Engine::ECS::Entity), SprayCanMonkey::chasing_on_update)
REGISTER_FUNCTION(spraycan_monkey_chasing_exit, void(Engine::ECS::Entity), SprayCanMonkey::chasing_on_exit)
REGISTER_FUNCTION(spraycan_monkey_windup_enter, void(Engine::ECS::Entity), SprayCanMonkey::windup_on_enter)
REGISTER_FUNCTION(spraycan_monkey_windup_update, void(Engine::ECS::Entity), SprayCanMonkey::windup_on_update)
REGISTER_FUNCTION(spraycan_monkey_spraying_enter, void(Engine::ECS::Entity), SprayCanMonkey::spraying_on_enter)
REGISTER_FUNCTION(spraycan_monkey_spraying_update, void(Engine::ECS::Entity), SprayCanMonkey::spraying_on_update)
REGISTER_FUNCTION(spraycan_monkey_spraying_exit, void(Engine::ECS::Entity), SprayCanMonkey::spraying_on_exit)
REGISTER_FUNCTION(spraycan_monkey_hit_enter, void(Engine::ECS::Entity), SprayCanMonkey::hit_on_enter)
REGISTER_FUNCTION(spraycan_monkey_hit_update, void(Engine::ECS::Entity), SprayCanMonkey::hit_on_update)
REGISTER_FUNCTION(spraycan_monkey_hit_exit, void(Engine::ECS::Entity), SprayCanMonkey::hit_on_exit)
REGISTER_FUNCTION(spraycan_monkey_death_enter, void(Engine::ECS::Entity), SprayCanMonkey::death_on_enter)
REGISTER_FUNCTION(spraycan_monkey_death_update, void(Engine::ECS::Entity), SprayCanMonkey::death_on_update)
REGISTER_FUNCTION(spraycan_monkey_death_exit, void(Engine::ECS::Entity), SprayCanMonkey::death_on_exit)
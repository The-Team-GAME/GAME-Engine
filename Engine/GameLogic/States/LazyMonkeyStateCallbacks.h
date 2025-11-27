//-----------------------------------------------------------------------------------
// file:    LazyMonkeyStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Lazy Monkey state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace LazyMonkeyStateCallbacks
{
  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter(Engine::ECS::Entity& entity) {
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    if (lazy_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      lazy_monkey->last_attack_time = Engine::Time::get_time(); // Reusing this field as "idle_enter_time"
    }
    std::cout << "Lazy Monkey: Entered Idle state" << std::endl;
  }

  inline void idle_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (!lazy_monkey) return;

    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - lazy_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    // Print error if player entity not found
    if (!player_entity.is_valid()) {
      std::cout << "Error: Player entity not found in Lazy Monkey Idle state update" << std::endl;
      return;
    }

    // Check if player is nearby
    if (player_entity.is_valid() && lazy_monkey) {
      auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (lazy_monkey_transform && player_transform) {
        float dx = player_transform->position.x - lazy_monkey_transform->position.x;
        float dy = player_transform->position.y - lazy_monkey_transform->position.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Start chasing if player is within detection range
        if (distance < lazy_monkey->detection_range) {
          state_machine->change_state("Chasing");
        }
      }
    }
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================

  inline void chasing_on_enter(Engine::ECS::Entity& entity) {
    std::cout << "Lazy Monkey: Entered Chasing state" << std::endl;
  }

  inline void chasing_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (!lazy_monkey) return;

    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid() && rigid && lazy_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - lazy_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally
        if (distance > 10.f) {
          rigid->velocity.x = (dx > 0 ? lazy_monkey->chase_speed : -lazy_monkey->chase_speed);
          // Update flip direction based on movement
          if (flip) {
            flip->value.x = (dx > 0 ? 1.0f : -1.0f);
          }
          // Don't modify velocity.y - let gravity handle it
        }

        // Return to idle if player is far away
        if (distance > lazy_monkey->chase_range) {
          state_machine->change_state("Idle");
        }

        // Attack if close enough AND cooldown has elapsed
        if (distance < lazy_monkey->attack_range &&
          (current_time - lazy_monkey->last_attack_time) >= lazy_monkey->attack_cooldown) {
          lazy_monkey->last_attack_time = current_time;
          state_machine->change_state("Attacking");
        }
      }
    }
  }

  inline void chasing_on_exit(Engine::ECS::Entity& entity) {

  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  inline void attacking_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (rigid && flip && lazy_monkey && rigid->on_ground) {
      // Throw monkey forward based on facing direction
      rigid->velocity.x = (flip->value.x > 0 ? lazy_monkey->leap_speed : -lazy_monkey->leap_speed);
      // Add upward velocity for a leap effect
      rigid->velocity.y = -lazy_monkey->leap_height; // Negative is up
    }

    std::cout << "Lazy Monkey: Entered Attacking state" << std::endl;
  }

  inline void attacking_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // Transition back to chasing when attack animation completes
    if (animation && animation->has_ended()) {
      state_machine->change_state("Chasing");
    }
  }

  inline void attacking_on_exit(Engine::ECS::Entity& entity) {
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    if (lazy_monkey) {
      // Update last attack time for cooldown
      lazy_monkey->last_attack_time = Engine::Time::get_time();
    }
  }

  //// ========================================================================
  //// Hit State Callbacks
  //// ========================================================================

  inline void hit_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    std::cout << "Lazy Monkey: Stunned!" << std::endl;
  }

  inline void hit_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!lazy_monkey) return;

    // When animation ends, detach from hitbox and apply knockback
    if (animation->has_ended() && pending_knockback && rigid) {
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
    if (animation->has_ended() && rigid && rigid->on_ground) {
      state_machine->change_state("Idle");
    }
  }

  inline void hit_on_exit(Engine::ECS::Entity& entity) {
    // Cleanup: ensure parent is removed (safety check)
    if (entity.has_component<Components::Parent>()) {
      entity.remove_component<Components::Parent>();
    }
  }

  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  inline void death_on_enter(Engine::ECS::Entity& entity) {
    std::cout << "Lazy Monkey: Died!" << std::endl;
  }

  inline void death_on_update(Engine::ECS::Entity& entity) {
    auto* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      std::cout << "Lazy Monkey: Destroyed after death animation." << std::endl;
    }
  }

  inline void death_on_exit(Engine::ECS::Entity& entity) {
    // Should not happen unless respawning
  }

} // namespace LazyMonkeyStateCallbacks

// Register all lazy monkey state callbacks
REGISTER_FUNCTION(lazy_monkey_idle_enter, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::idle_on_enter)
REGISTER_FUNCTION(lazy_monkey_idle_update, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::idle_on_update)
REGISTER_FUNCTION(lazy_monkey_chasing_enter, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::chasing_on_enter)
REGISTER_FUNCTION(lazy_monkey_chasing_update, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::chasing_on_update)
REGISTER_FUNCTION(lazy_monkey_chasing_exit, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::chasing_on_exit)
REGISTER_FUNCTION(lazy_monkey_attacking_enter, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::attacking_on_enter)
REGISTER_FUNCTION(lazy_monkey_attacking_update, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::attacking_on_update)
REGISTER_FUNCTION(lazy_monkey_attacking_exit, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::attacking_on_exit)
REGISTER_FUNCTION(lazy_monkey_hit_enter, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::hit_on_enter)
REGISTER_FUNCTION(lazy_monkey_hit_update, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::hit_on_update)
REGISTER_FUNCTION(lazy_monkey_hit_exit, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::hit_on_exit)
REGISTER_FUNCTION(lazy_monkey_death_enter, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::death_on_enter)
REGISTER_FUNCTION(lazy_monkey_death_update, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::death_on_update)
REGISTER_FUNCTION(lazy_monkey_death_exit, void(Engine::ECS::Entity&), LazyMonkeyStateCallbacks::death_on_exit)

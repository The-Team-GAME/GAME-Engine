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
    std::cout << "Lazy Monkey: Entered Idle state" << std::endl;
  }

  inline void idle_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

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
            flip->value.x = (dx > 0 ? -1.0f : 1.0f); // TODO: Unflip this when sprite is fixed
          }
          // Don't modify velocity.y - let gravity handle it
        }
        else {
          rigid->velocity.x = 0.f;
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
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      rigid->velocity.x = 0.f;
      // Don't reset velocity.y - let gravity/physics handle it
    }
  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  inline void attacking_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (rigid && flip && lazy_monkey) {
      // Throw monkey forward based on facing direction
      rigid->velocity.x = (flip->value.x > 0 ? -lazy_monkey->leap_speed : lazy_monkey->leap_speed); // TODO: Unflip this when sprite is fixed
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

  // ========================================================================
  // Hit State Callbacks
  // ========================================================================

  //inline void hit_on_enter(Engine::ECS::Entity& entity) {
  //  auto* rigid = entity.get_component<Components::RigidBody>();
  //  if (rigid) {
  //    rigid->velocity.x = 0.f;
  //    // Don't reset velocity.y - let gravity/physics handle it
  //  }

  //  std::cout << "Lazy Monkey: Got hit!" << std::endl;
  //}

  //inline void hit_on_update(Engine::ECS::Entity& entity) {
  //  auto* state_machine = entity.get_component<Components::StateMachine>();
  //  auto* animation = entity.get_component<Components::Animation>();

  //  // Transition to idle when hit animation completes
  //  if (animation && animation->has_ended()) {
  //    state_machine->change_state("Idle");
  //  }
  //}

  //// ========================================================================
  //// Stunned State Callbacks
  //// ========================================================================

  inline void hit_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (rigid && flip && lazy_monkey) {
      // Apply knockback force in opposite direction of facing
      // If flipped left (flip.x < 0), knockback goes right (positive)
      // If flipped right (flip.x > 0), knockback goes left (negative)
      rigid->velocity.x = (flip->value.x > 0 ? lazy_monkey->knockback_force : -lazy_monkey->knockback_force);
      rigid->velocity.y = -lazy_monkey->knockback_upward; // Negative is up

      // Reset stun timer
      lazy_monkey->stun_timer = 0.0f;
    }

    std::cout << "Lazy Monkey: Stunned!" << std::endl;
  }

  inline void hit_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    if (!lazy_monkey) return;

    // Update stun timer
    lazy_monkey->stun_timer += Engine::Time::get_fixed_dt();

    // Apply friction to knockback velocity
    if (rigid) {
      rigid->velocity.x *= 0.95f;
    }

    // Transition to idle when stun duration expires
    if (lazy_monkey->stun_timer >= lazy_monkey->stun_duration) {
      state_machine->change_state("Idle");
    }
  }

  inline void hit_on_exit(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      rigid->velocity.x = 0.f;
    }
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

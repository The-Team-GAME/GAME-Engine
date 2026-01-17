//-----------------------------------------------------------------------------------
// file:    GamerMonkeyStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Gamer Monkey state machine callback implementations (serializable)
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace GamerMonkeyStateCallbacks
{
  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter(Engine::ECS::Entity& entity) {
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    if (Gamer_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      Gamer_monkey->last_attack_time = Engine::Time::get_time(); // Reusing this field as "idle_enter_time"
    }
    Log::info("Gamer Monkey: Entered Idle state");
  }

  inline void idle_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    if (!Gamer_monkey) return;

    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - Gamer_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!player_entity.is_valid()) {
      return;
    }

    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* player_transform = player_entity.get_component<Components::Transform2D>();

    if (Gamer_monkey_transform && player_transform) {
      float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
      float dy = player_transform->position.y - Gamer_monkey_transform->position.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      // Start chasing if player is within normal detection range,
      // otherwise start a long-range dash if within the extended chase range
      if (distance < Gamer_monkey->detection_range) {
        state_machine->change_state("Chasing");
      }
      else if (distance < Gamer_monkey->chase_range) {
        state_machine->change_state("Dash");
      }
    }
  }

  // ========================================================================
  // Dashing State Callbacks
  // ========================================================================
  inline void dashing_on_enter(Engine::ECS::Entity& entity) {
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();

    // Face the player immediately when starting the dash
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (player_entity.is_valid() && Gamer_monkey_transform && flip) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();
      if (player_transform) {
        float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
        flip->value.x = (dx >= 0.0f ? 1.0f : -1.0f);
      }
    }

    // Make the monkey itself deal 1 damage on collision while dashing
    entity.add_component<Components::Attack>(1); // fixed 1 damage for dash
    entity.add_to_layer("damage_player");

    Log::info("Gamer Monkey: Entered Dash state");
  }

  inline void dashing_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    if (!Gamer_monkey || !state_machine) return;

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid() || !rigid || !Gamer_monkey_transform) {
      state_machine->change_state("Idle");
      return;
    }

    auto* player_transform = player_entity.get_component<Components::Transform2D>();
    if (!player_transform) {
      state_machine->change_state("Idle");
      return;
    }

    float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
    float distance = std::abs(dx); // Only horizontal distance for sidescrolling

    // Dash faster than normal chase speed towards the player
    float dash_speed = Gamer_monkey->chase_speed * 3.0f;
    if (rigid) {
      rigid->velocity.x = (dx > 0.0f ? dash_speed : -dash_speed);
    }

    if (flip) {
      flip->value.x = (dx >= 0.0f ? 1.0f : -1.0f);
    }

    // If we have closed the gap enough, fall back to normal chasing
    if (distance <= Gamer_monkey->detection_range) {
      state_machine->change_state("Chasing");
      return;
    }

    // If player is now too far, stop dashing and go idle
    if (distance > Gamer_monkey->chase_range) {
      state_machine->change_state("Idle");
      return;
    }
  }

  inline void dashing_on_exit(Engine::ECS::Entity& entity) {
    // Remove dash attack effect and damage layer
    if (entity.has_component<Components::Attack>()) {
      entity.remove_component<Components::Attack>();
    }
    entity.remove_from_layer("damage_player");
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================
  inline void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity& entity) {
    Log::info("Gamer Monkey: Entered Chasing state");
  }

  inline void chasing_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    if (!Gamer_monkey) return;

    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid() && rigid && Gamer_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally, but stop when within attack range
        if (distance > Gamer_monkey->close_attack_range) {
          rigid->velocity.x = (dx > 0 ? Gamer_monkey->chase_speed : -Gamer_monkey->chase_speed);
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

        // If player moves out of normal chase detection but is still within
        // extended range, switch to dash for a long-range approach
        if (distance > Gamer_monkey->detection_range && distance <= Gamer_monkey->chase_range) {
          state_machine->change_state("Dash");
          return;
        }

        // Return to idle if player is completely out of range
        if (distance > Gamer_monkey->chase_range) {
          state_machine->change_state("Idle");
        }

        // If close enough and cooldown has elapsed, go to windup before attacking
        if (distance < Gamer_monkey->close_attack_range &&
          (current_time - Gamer_monkey->last_attack_time) >= Gamer_monkey->attack_cooldown) {
          state_machine->change_state("Windup");
        }
      }
    }
  }

  inline void chasing_on_exit([[maybe_unused]] Engine::ECS::Entity& entity) {

  }

  // ========================================================================
  // Windup State Callbacks
  // ========================================================================

  inline void windup_on_enter(Engine::ECS::Entity& entity) {
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when windup started (reuse last_attack_time)
    if (Gamer_monkey) {
      Gamer_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Gamer Monkey: Winding up for attack");
  }

  inline void windup_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!Gamer_monkey) return;

    float current_time = Engine::Time::get_time();
    float windup_elapsed = current_time - Gamer_monkey->last_attack_time;

    // Get player entity to face them during windup
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid()) {
      auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (Gamer_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
        // Update flip direction to face player during wind-up
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // After windup duration, attack
    if (windup_elapsed >= Gamer_monkey->attack_windup) {
      Gamer_monkey->last_attack_time = current_time;
      state_machine->change_state("Attacking");
    }
  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  inline void attacking_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    // auto* transform = entity.get_component<Components::Transform2D>();

    if (rigid && flip && Gamer_monkey && rigid->on_ground) {
      // Throw monkey forward based on facing direction
      rigid->velocity.x = (flip->value.x > 0 ? Gamer_monkey->leap_speed : -Gamer_monkey->leap_speed);
      // Add upward velocity for a leap effect
      rigid->velocity.y = -Gamer_monkey->leap_height; // Negative is up
    }

    // Make the monkey itself the hitbox
    if (Gamer_monkey && flip) {
      // Add Attack component to the monkey
      // Note: We use the monkey's own collider for the attack hitbox now
      entity.add_component<Components::Attack>(Gamer_monkey->damage);
      auto* attack = entity.get_component<Components::Attack>();

      if (attack) {
        // Apply knockback direction based on facing
        attack->knockback = Gamer_monkey->knockback;
        if (flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);
      }

      // Add to damage_player layer so it collides with player
      // We KEEP the enemy layer so it still collides with ground
      entity.add_to_layer("damage_player");
    }

    Log::info("Gamer Monkey: Entered Attacking state");
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
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    if (Gamer_monkey) {
      // Update last attack time for cooldown
      Gamer_monkey->last_attack_time = Engine::Time::get_time();
    }

    // Remove Attack component and damage_player layer
    if (entity.has_component<Components::Attack>()) {
      entity.remove_component<Components::Attack>();
    }
    entity.remove_from_layer("damage_player");
  }


  inline void hit_on_enter(Engine::ECS::Entity& entity) {
    (void)entity; // Suppress unused parameter warning
    Log::info("Gamer Monkey: Stunned!");
  }

  inline void hit_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!Gamer_monkey) return;

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

  inline void death_on_enter([[maybe_unused]] Engine::ECS::Entity& entity) {
    Log::info("Gamer Monkey: Died!");
  }

  inline void death_on_update(Engine::ECS::Entity& entity) {
    auto* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      Log::info("Gamer Monkey: Destroyed after death animation.");
    }
  }

  inline void death_on_exit([[maybe_unused]] Engine::ECS::Entity& entity) {
    // Should not happen unless respawning
  }

} // namespace GamerMonkeyStateCallbacks

// Register all Gamer monkey state callbacks
REGISTER_FUNCTION(Gamer_monkey_idle_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::idle_on_enter)
REGISTER_FUNCTION(Gamer_monkey_idle_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::idle_on_update)
REGISTER_FUNCTION(Gamer_monkey_dashing_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::dashing_on_enter)
REGISTER_FUNCTION(Gamer_monkey_dashing_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::dashing_on_update)
REGISTER_FUNCTION(Gamer_monkey_dashing_exit, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::dashing_on_exit)
REGISTER_FUNCTION(Gamer_monkey_chasing_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::chasing_on_enter)
REGISTER_FUNCTION(Gamer_monkey_chasing_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::chasing_on_update)
REGISTER_FUNCTION(Gamer_monkey_chasing_exit, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::chasing_on_exit)
REGISTER_FUNCTION(Gamer_monkey_windup_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::windup_on_enter)
REGISTER_FUNCTION(Gamer_monkey_windup_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::windup_on_update)
REGISTER_FUNCTION(Gamer_monkey_attacking_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::attacking_on_enter)
REGISTER_FUNCTION(Gamer_monkey_attacking_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::attacking_on_update)
REGISTER_FUNCTION(Gamer_monkey_attacking_exit, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::attacking_on_exit)
REGISTER_FUNCTION(Gamer_monkey_hit_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::hit_on_enter)
REGISTER_FUNCTION(Gamer_monkey_hit_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::hit_on_update)
REGISTER_FUNCTION(Gamer_monkey_hit_exit, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::hit_on_exit)
REGISTER_FUNCTION(Gamer_monkey_death_enter, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::death_on_enter)
REGISTER_FUNCTION(Gamer_monkey_death_update, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::death_on_update)
REGISTER_FUNCTION(Gamer_monkey_death_exit, void(Engine::ECS::Entity&), GamerMonkeyStateCallbacks::death_on_exit)

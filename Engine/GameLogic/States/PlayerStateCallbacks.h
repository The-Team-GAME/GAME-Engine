//-----------------------------------------------------------------------------------
// file:    PlayerStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Player state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"

namespace PlayerStateCallbacks
{
  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) rigid->velocity.x = 0.f;
    std::cout << "Player: Entered Idle state" << std::endl;
  }

  inline void idle_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();

    // Check for horizontal movement input (A/D only for sidescrolling)
    if (Engine::Input::is_held(Engine::KeyCode::A) ||
      Engine::Input::is_held(Engine::KeyCode::D))
    {
      state_machine->change_state("Walking");
    }

    // Check for jump input
    if (Engine::Input::is_pressed(Engine::KeyCode::Space))
    {
      auto* rigid = entity.get_component<Components::RigidBody>();
      // Only jump if on ground
      if (rigid && rigid->on_ground) {
        state_machine->change_state("Jumping");
      }
    }

    // Check for attack input
    if (Engine::Input::is_pressed(Engine::KeyCode::F))
    {
      state_machine->change_state("Attack1");
    }
  }

  // ========================================================================
  // Walking State Callbacks
  // ========================================================================

  inline void walking_on_enter(Engine::ECS::Entity& entity) {
    std::cout << "Player: Entered Walking state" << std::endl;
  }

  inline void walking_on_update(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player = entity.get_component<Components::Player>();

    if (!player) return;

    rigid->velocity.x = 0.f;
    // Don't touch velocity.y - let gravity handle it

    // Horizontal movement only (sidescrolling)
    if (Engine::Input::is_held(Engine::KeyCode::A)) {
      rigid->velocity.x = -player->walk_speed;
      if (flip) flip->value.x = -1.0f; // Face left
    }
    if (Engine::Input::is_held(Engine::KeyCode::D)) {
      rigid->velocity.x = player->walk_speed;
      if (flip) flip->value.x = 1.0f; // Face right
    }

    // Return to idle if no horizontal input
    if (!Engine::Input::is_held(Engine::KeyCode::A) &&
      !Engine::Input::is_held(Engine::KeyCode::D))
    {
      state_machine->change_state("Idle");
    }

    // Check for jump input
    if (Engine::Input::is_pressed(Engine::KeyCode::Space))
    {
      // Only jump if on ground
      if (rigid && rigid->on_ground) {
        state_machine->change_state("Jumping");
      }
    }

    // Can attack while moving
    if (Engine::Input::is_pressed(Engine::KeyCode::F))
    {
      state_machine->change_state("Attack1");
    }
  }

  inline void walking_on_exit(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      rigid->velocity.x = 0.f;
      // Don't reset velocity.y - let gravity/physics handle it
    }
  }

  // ========================================================================
  // Jumping State Callbacks
  // ========================================================================

  inline void jumping_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* player = entity.get_component<Components::Player>();

    if (rigid && player) {
      // Apply jump force
      rigid->velocity.y = -player->jump_force; // Negative is up
    }

    std::cout << "Player: Entered Jumping state" << std::endl;
  }

  inline void jumping_on_update(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player = entity.get_component<Components::Player>();

    if (!player) return;

    // Allow horizontal air control
    rigid->velocity.x = 0.f;

    if (Engine::Input::is_held(Engine::KeyCode::A)) {
      rigid->velocity.x = -player->air_speed;
      if (flip) flip->value.x = -1.0f; // Face left
    }
    if (Engine::Input::is_held(Engine::KeyCode::D)) {
      rigid->velocity.x = player->air_speed;
      if (flip) flip->value.x = 1.0f; // Face right
    }

    // Return to idle when landing
    if (rigid && rigid->on_ground) {
      // Check if moving horizontally
      if (Engine::Input::is_held(Engine::KeyCode::A) ||
        Engine::Input::is_held(Engine::KeyCode::D))
      {
        state_machine->change_state("Walking");
      }
      else {
        state_machine->change_state("Idle");
      }
    }

    // Can attack while jumping
    if (Engine::Input::is_pressed(Engine::KeyCode::F))
    {
      state_machine->change_state("Attack1");
    }
  }

  // ========================================================================
  // Attack 1 State Callbacks (First Punch)
  // ========================================================================

  inline void attack1_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player = entity.get_component<Components::Player>();
    auto* heartrate = entity.get_component<Components::Heartrate>();

    // Increase heartrate
    if (heartrate) {
      heartrate->increase(player->attack1.heartrate_cost);
    }

    // Apply forward movement during attack
    if (rigid && flip && player) {
      rigid->velocity.x = (flip->value.x < 0) ? -player->attack1.dash_speed : player->attack1.dash_speed;
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();

    if (transform && player) {
      float offset_x = (flip && flip->value.x < 0) ? -player->attack1.hitbox_offset_x : player->attack1.hitbox_offset_x;
      float offset_y = player->attack1.hitbox_offset_y;

      auto hitbox = Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Attack>(player->attack1.damage)
        .add_component<Components::Transform2D>(
          transform->position.x + offset_x,
          transform->position.y + offset_y,
          0.0f,
          player->attack1.hitbox_width,
          player->attack1.hitbox_height
        )
        .add_component<Components::RigidBody>(true, false)
        .add_component<Components::BoxCollider>(player->attack1.hitbox_width, player->attack1.hitbox_height, false)
        .add_component<Components::Parent>(entity)
        .add_component<Components::Lifetime>(0.48f) // Attack1 duration: 6 frames × 0.08f
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");

      // Apply knockback direction based on facing
      auto* attack = hitbox.get_component<Components::Attack>();
      if (attack) {
        attack->knockback = player->attack1.knockback;
        if (flip && flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);
      }
    }

    std::cout << "Player: Attack 1 (First Punch)" << std::endl;
  }

  inline void attack1_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    if (!animation) return;

    // Check if player pressed attack again during this combo window
    if (Engine::Input::is_pressed(Engine::KeyCode::F) && !animation->has_ended()) {
      // Queue up next attack
      if (animation->get_current_frame_index() >= 3) { // Allow combo after frame 3
        state_machine->change_state("Attack2");
      }
    }

    // Transition to idle when attack animation completes
    if (animation->has_ended()) {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      // offset slightly forward because attack texture is wider than idle texture
      if (transform && flip) {
        transform->position.x += (flip->value.x < 0) ? 20.0f : -20.0f;
      }

      // Force so that we don't display the attack texture after offset is applied
      state_machine->change_state("Idle");
    }
  }

  // ========================================================================
  // Attack 2 State Callbacks (Second Punch)
  // ========================================================================

  inline void attack2_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player = entity.get_component<Components::Player>();
    auto* heartrate = entity.get_component<Components::Heartrate>();

    // Increase heartrate
    if (heartrate) {
      heartrate->increase(player->attack2.heartrate_cost);
    }

    // Apply forward movement during attack
    if (rigid && flip && player) {
      rigid->velocity.x = (flip->value.x < 0) ? -player->attack2.dash_speed : player->attack2.dash_speed;
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();

    if (transform && player) {
      float offset_x = (flip && flip->value.x < 0) ? -player->attack2.hitbox_offset_x : player->attack2.hitbox_offset_x;
      float offset_y = player->attack2.hitbox_offset_y;

      auto hitbox = Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Attack>(player->attack2.damage)
        .add_component<Components::Transform2D>(
          transform->position.x + offset_x,
          transform->position.y + offset_y,
          0.0f,
          player->attack2.hitbox_width,
          player->attack2.hitbox_height
        )
        .add_component<Components::RigidBody>(true, false)
        .add_component<Components::BoxCollider>(player->attack2.hitbox_width, player->attack2.hitbox_height, false)
        .add_component<Components::Parent>(entity)
        .add_component<Components::Lifetime>(0.24f) // Attack2 duration: 3 frames × 0.08f
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");

      // Apply knockback direction based on facing
      auto* attack = hitbox.get_component<Components::Attack>();
      if (attack) {
        attack->knockback = player->attack2.knockback;
        if (flip && flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);
      }
    }

    std::cout << "Player: Attack 2 (Second Punch)" << std::endl;
  }

  inline void attack2_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    if (!animation) return;

    // Check if player pressed attack again during this combo window
    if (Engine::Input::is_pressed(Engine::KeyCode::F) && !animation->has_ended()) {
      // Allow combo after frame 1 of this attack
      if (animation->get_current_frame_index() >= 1) {
        state_machine->change_state("Attack3");
      }
    }

    // Transition to idle when attack animation completes
    if (animation->has_ended()) {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      // offset slightly forward because attack texture is wider than idle texture
      if (transform && flip) {
        transform->position.x += (flip->value.x < 0) ? 20.0f : -20.0f;
      }

      // Force so that we don't display the attack texture after offset is applied
      state_machine->change_state("Idle");
    }
  }

  // ========================================================================
  // Attack 3 State Callbacks (Third Punch - Finisher)
  // ========================================================================

  inline void attack3_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player = entity.get_component<Components::Player>();
    auto* heartrate = entity.get_component<Components::Heartrate>();

    // Increase heartrate
    if (heartrate) {
      heartrate->increase(player->attack3.heartrate_cost);
    }

    // Apply forward movement during attack
    if (rigid && flip && player) {
      rigid->velocity.x = (flip->value.x < 0) ? -player->attack3.dash_speed : player->attack3.dash_speed;
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();

    if (transform && player) {
      float offset_x = (flip && flip->value.x < 0) ? -player->attack3.hitbox_offset_x : player->attack3.hitbox_offset_x;
      float offset_y = player->attack3.hitbox_offset_y;

      auto hitbox = Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Attack>(player->attack3.damage)
        .add_component<Components::Transform2D>(
          transform->position.x + offset_x,
          transform->position.y + offset_y,
          0.0f,
          player->attack3.hitbox_width,
          player->attack3.hitbox_height
        )
        .add_component<Components::RigidBody>(true, false)
        .add_component<Components::BoxCollider>(player->attack3.hitbox_width, player->attack3.hitbox_height, false)
        .add_component<Components::Parent>(entity)
        .add_component<Components::Lifetime>(0.6f) // Attack3 duration: 10 frames × 0.06f
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");

      // Apply knockback direction based on facing
      auto* attack = hitbox.get_component<Components::Attack>();
      if (attack) {
        attack->knockback = player->attack3.knockback;
        if (flip && flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);
      }
    }

    std::cout << "Player: Attack 3 (Third Punch - Finisher!)" << std::endl;
  }

  inline void attack3_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // No combo after this - it's the finisher
    // Just wait for animation to complete
    if (animation && animation->has_ended()) {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      // offset slightly forward because attack texture is wider than idle texture
      if (transform && flip) {
        transform->position.x += (flip->value.x < 0) ? 20.0f : -20.0f;
      }

      // Force so that we don't display the attack texture after offset is applied
      state_machine->change_state("Idle");
    }
  }

} // namespace PlayerStateCallbacks

// Register all player state callbacks
REGISTER_FUNCTION(player_idle_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::idle_on_enter)
REGISTER_FUNCTION(player_idle_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::idle_on_update)
REGISTER_FUNCTION(player_walking_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::walking_on_enter)
REGISTER_FUNCTION(player_walking_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::walking_on_update)
REGISTER_FUNCTION(player_walking_exit, void(Engine::ECS::Entity&), PlayerStateCallbacks::walking_on_exit)
REGISTER_FUNCTION(player_jumping_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::jumping_on_enter)
REGISTER_FUNCTION(player_jumping_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::jumping_on_update)
REGISTER_FUNCTION(player_attack1_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::attack1_on_enter)
REGISTER_FUNCTION(player_attack1_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::attack1_on_update)
REGISTER_FUNCTION(player_attack2_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::attack2_on_enter)
REGISTER_FUNCTION(player_attack2_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::attack2_on_update)
REGISTER_FUNCTION(player_attack3_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::attack3_on_enter)
REGISTER_FUNCTION(player_attack3_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::attack3_on_update)

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
      state_machine->change_state("Attacking");
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
      state_machine->change_state("Attacking");
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
      state_machine->change_state("Attacking");
    }
  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  inline void attacking_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      rigid->velocity.x = 0.f;
      // Don't reset velocity.y - let gravity/physics handle it
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player = entity.get_component<Components::Player>();

    if (transform && player) {
      // Position hitbox in front of player based on facing direction
      float hitbox_offset = (flip && flip->value.x < 0) ? -player->hitbox_offset : player->hitbox_offset;

      Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Transform2D>(
          transform->position.x + hitbox_offset,
          transform->position.y,
          0.0f,
          player->hitbox_width,
          player->hitbox_height
        )
        .add_component<Components::RigidBody>(true, false) // Static (not affected by forces), no gravity
        .add_component<Components::BoxCollider>(player->hitbox_width, player->hitbox_height, false) // Not a static obstacle
        .add_component<Components::FrameLifetime>(1) // Destroy after 1 frame
        .add_component<Components::Color>(Math::Vec4D(1.0f, 0.0f, 0.0f, 0.3f)) // Semi-transparent red for debugging
        .add_component<Components::Texture>(Engine::Graphics::get_rect_texture())
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");
    }

    std::cout << "Player: Entered Attacking state" << std::endl;
  }

  inline void attacking_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // Transition to idle when attack animation completes
    if (animation && animation->has_ended()) {
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
REGISTER_FUNCTION(player_attacking_enter, void(Engine::ECS::Entity&), PlayerStateCallbacks::attacking_on_enter)
REGISTER_FUNCTION(player_attacking_update, void(Engine::ECS::Entity&), PlayerStateCallbacks::attacking_on_update)

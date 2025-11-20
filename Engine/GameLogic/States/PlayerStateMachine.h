//-----------------------------------------------------------------------------------
// file:    PlayerStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the player state machine and all player states
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

namespace PlayerStates
{
  // Player dimensions
  constexpr f32 IDLE_WIDTH = 64.0f;
  constexpr f32 IDLE_HEIGHT = 64.0f;
  constexpr f32 ATTACK_WIDTH = 96.0f;
  constexpr f32 ATTACK_HEIGHT = 64.0f;

  inline Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Player Idle State
    // ========================================================================
    Components::StateAnimation player_idle_anim;
    player_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"));
    player_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 24, 24);
    player_idle_anim.animation = Components::Animation(24, 0.1f);
    player_idle_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_idle;
    player_idle.name = "Idle";
    player_idle.on_enter = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      if (rigid) rigid->velocity.x = 0.f;

      std::cout << "Player: Entered Idle state" << std::endl;
      };
    player_idle.on_update = [](Engine::ECS::Entity& entity) {
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
      };
    player_idle.on_exit = nullptr;
    player_idle.animation = player_idle_anim;

    // ========================================================================
    // Player Walking State
    // ========================================================================
    Components::StateAnimation player_walking_anim;
    player_walking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_walking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_walking_anim.animation = Components::Animation(8, 0.1f);
    player_walking_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_walking;
    player_walking.name = "Walking";
    player_walking.on_enter = [](Engine::ECS::Entity& entity) {
      std::cout << "Player: Entered Walking state" << std::endl;
      };
    player_walking.on_update = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* flip = entity.get_component<Components::Flip>();

      float speed = 600.f;
      rigid->velocity.x = 0.f;
      // Don't touch velocity.y - let gravity handle it

      // Horizontal movement only (sidescrolling)
      if (Engine::Input::is_held(Engine::KeyCode::A)) {
        rigid->velocity.x = -speed;
        if (flip) flip->value.x = -1.0f; // Face left
      }
      if (Engine::Input::is_held(Engine::KeyCode::D)) {
        rigid->velocity.x = speed;
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
      };
    player_walking.on_exit = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      if (rigid) {
        rigid->velocity.x = 0.f;
        // Don't reset velocity.y - let gravity/physics handle it
      }
      };
    player_walking.animation = player_walking_anim;

    // ========================================================================
    // Player Jumping State
    // ========================================================================
    Components::StateAnimation player_jumping_anim;
    player_jumping_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_jumping_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_jumping_anim.animation = Components::Animation(8, 0.1f);
    player_jumping_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_jumping;
    player_jumping.name = "Jumping";
    player_jumping.on_enter = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      if (rigid) {
        // Apply jump force
        rigid->velocity.y = -900.f; // Negative is up
      }

      std::cout << "Player: Entered Jumping state" << std::endl;
      };
    player_jumping.on_update = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* flip = entity.get_component<Components::Flip>();

      // Allow horizontal air control
      float air_speed = 150.f; // Slightly slower than ground movement
      rigid->velocity.x = 0.f;

      if (Engine::Input::is_held(Engine::KeyCode::A)) {
        rigid->velocity.x = -air_speed;
        if (flip) flip->value.x = -1.0f; // Face left
      }
      if (Engine::Input::is_held(Engine::KeyCode::D)) {
        rigid->velocity.x = air_speed;
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
      };
    player_jumping.on_exit = nullptr;
    player_jumping.animation = player_jumping_anim;

    // ========================================================================
    // Player Attacking State
    // ========================================================================
    Components::StateAnimation player_attacking_anim;
    player_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_threehit_spritesheet.png"));
    player_attacking_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 19, 6);
    player_attacking_anim.animation = Components::Animation(6, 0.05f);
    player_attacking_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attacking;
    player_attacking.name = "Attacking";
    player_attacking.on_enter = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      if (rigid) {
        rigid->velocity.x = 0.f;
        // Don't reset velocity.y - let gravity/physics handle it
      }

      // Create attack hitbox entity
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      if (transform) {
        // Position hitbox in front of player based on facing direction
        float hitbox_offset = (flip && flip->value.x < 0) ? -80.0f : 80.0f;
        float hitbox_width = 100.0f;
        float hitbox_height = ATTACK_HEIGHT * 2.0f;

        Engine::ECS::create_entity()
          .add_component<Components::Tag<"PlayerAttackHitbox">>()
          .add_component<Components::Transform2D>(
            transform->position.x + hitbox_offset,
            transform->position.y,
            0.0f,
            hitbox_width,
            hitbox_height
          )
          .add_component<Components::RigidBody>(true, false) // Static (not affected by forces), no gravity
          .add_component<Components::BoxCollider>(hitbox_width, hitbox_height, false) // Not a static obstacle
          .add_component<Components::FrameLifetime>(1) // Destroy after 1 frame
          .add_component<Components::Color>(Math::Vec4D(1.0f, 0.0f, 0.0f, 0.3f)) // Semi-transparent red for debugging
          .add_component<Components::Texture>(Engine::Graphics::get_rect_texture())
          .remove_from_layer("default")
          .add_to_layer("damage_enemy");
      }

      std::cout << "Player: Entered Attacking state" << std::endl;
      };
    player_attacking.on_update = [](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* animation = entity.get_component<Components::Animation>();

      // Transition to idle when attack animation completes
      if (animation && animation->has_ended()) {
        state_machine->change_state("Idle");
      }
      };
    player_attacking.on_exit = nullptr;
    player_attacking.animation = player_attacking_anim;

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine player_sm;
    player_sm.add_state(player_idle);
    player_sm.add_state(player_walking);
    player_sm.add_state(player_jumping);
    player_sm.add_state(player_attacking);
    player_sm.set_initial_state("Idle");

    return player_sm;
  }
}

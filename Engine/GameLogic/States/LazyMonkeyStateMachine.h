//-----------------------------------------------------------------------------------
// file:    LazyMonkeyStateMachine.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the lazy monkey state machine and all lazy monkey states
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

#include "../Components/GameplayComponent.h"

namespace LazyMonkeyStates
{
  // Lazy Monkey dimensions
  constexpr f32 WIDTH = 64.0f;
  constexpr f32 HEIGHT = 64.0f;

  inline Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Lazy Monkey Idle State
    // ========================================================================
    Components::StateAnimation lazy_monkey_idle_anim;
    lazy_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"));
    lazy_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    lazy_monkey_idle_anim.animation = Components::Animation(5, 0.15f);
    lazy_monkey_idle_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_idle;
    lazy_monkey_idle.name = "Idle";
    lazy_monkey_idle.on_enter = [](Engine::ECS::Entity& entity) {
      std::cout << "Lazy Monkey: Entered Idle state" << std::endl;
      };
    lazy_monkey_idle.on_update = [](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();

      // Get player entity
      auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

      // Print error if player entity not found
      if (!player_entity.is_valid()) {
        std::cout << "Error: Player entity not found in Lazy Monkey Idle state update" << std::endl;
        return;
      }

      // Check if player is nearby
      if (player_entity.is_valid()) {
        auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
        auto* player_transform = player_entity.get_component<Components::Transform2D>();

        if (lazy_monkey_transform && player_transform) {
          float dx = player_transform->position.x - lazy_monkey_transform->position.x;
          float dy = player_transform->position.y - lazy_monkey_transform->position.y;
          float distance = std::sqrt(dx * dx + dy * dy);

          // Start chasing if player is within 300 units
          if (distance < 300.f) {
            state_machine->change_state("Chasing");
          }
        }
      }
      };
    lazy_monkey_idle.on_exit = nullptr;
    lazy_monkey_idle.animation = lazy_monkey_idle_anim;

    // ========================================================================
    // Lazy Monkey Chasing State
    // ========================================================================
    Components::StateAnimation lazy_monkey_chasing_anim;
    lazy_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Walk_Spritesheet.png"));
    lazy_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    lazy_monkey_chasing_anim.animation = Components::Animation(6, 0.1f);
    lazy_monkey_chasing_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_chasing;
    lazy_monkey_chasing.name = "Chasing";
    lazy_monkey_chasing.on_enter = [](Engine::ECS::Entity& entity) {
      std::cout << "Lazy Monkey: Entered Chasing state" << std::endl;
      };
    lazy_monkey_chasing.on_update = [](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* rigid = entity.get_component<Components::RigidBody>();
      auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();

      // Attack cooldown tracking (in seconds)
      static float last_attack_time = 0.0f;
      const float attack_cooldown = 2.0f; // 2 seconds between attacks
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
            float speed = 100.f;
            rigid->velocity.x = (dx > 0 ? speed : -speed);
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
          if (distance > 400.f) {
            state_machine->change_state("Idle");
          }

          // Attack if close enough AND cooldown has elapsed
          if (distance < 80.f && (current_time - last_attack_time) >= attack_cooldown) {
            last_attack_time = current_time;
            state_machine->change_state("Attacking");
          }
        }
      }
      };
    lazy_monkey_chasing.on_exit = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      if (rigid) {
        rigid->velocity.x = 0.f;
        // Don't reset velocity.y - let gravity/physics handle it
      }
      };
    lazy_monkey_chasing.animation = lazy_monkey_chasing_anim;

    // ========================================================================
    // Lazy Monkey Attacking State
    // ========================================================================
    Components::StateAnimation lazy_monkey_attacking_anim;
    lazy_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Attack_Spritesheet.png"));
    lazy_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 4, 4);
    lazy_monkey_attacking_anim.animation = Components::Animation({ 0.2f, 0.1f, 0.1f, 0.5f }); // Custom frame durations
    lazy_monkey_attacking_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_attacking;
    lazy_monkey_attacking.name = "Attacking";
    lazy_monkey_attacking.on_enter = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      auto* flip = entity.get_component<Components::Flip>();

      if (rigid && flip) {
        // Throw monkey forward based on facing direction
        float leap_speed = 400.f;
        rigid->velocity.x = (flip->value.x > 0 ? -leap_speed : leap_speed); // TODO: Unflip this when sprite is fixed
        // Optional: Add slight upward velocity for a leap effect
        rigid->velocity.y = -300.f; // Negative is up
      }

      std::cout << "Lazy Monkey: Entered Attacking state" << std::endl;
      };
    lazy_monkey_attacking.on_update = [](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* animation = entity.get_component<Components::Animation>();

      // Transition back to chasing when attack animation completes
      if (animation && animation->has_ended()) {
        state_machine->change_state("Chasing");
      }
      };
    lazy_monkey_attacking.on_exit = [](Engine::ECS::Entity& entity) {
      };
    lazy_monkey_attacking.animation = lazy_monkey_attacking_anim;

    // ========================================================================
    // Lazy Monkey Hit State
    // ========================================================================
    Components::StateAnimation lazy_monkey_hit_anim;
    lazy_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"));
    lazy_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 3, 3);
    lazy_monkey_hit_anim.animation = Components::Animation(3, 0.1f);
    lazy_monkey_hit_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_hit;
    lazy_monkey_hit.name = "Hit";
    lazy_monkey_hit.on_enter = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      if (rigid) {
        rigid->velocity.x = 0.f;
        // Don't reset velocity.y - let gravity/physics handle it
      }

      std::cout << "Lazy Monkey: Got hit!" << std::endl;
      };
    lazy_monkey_hit.on_update = [](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* animation = entity.get_component<Components::Animation>();

      // Transition to idle when hit animation completes
      if (animation && animation->has_ended()) {
        state_machine->change_state("Idle");
      }
      };
    lazy_monkey_hit.on_exit = nullptr;
    lazy_monkey_hit.animation = lazy_monkey_hit_anim;

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine lazy_monkey_sm;
    lazy_monkey_sm.add_state(lazy_monkey_idle);
    lazy_monkey_sm.add_state(lazy_monkey_chasing);
    lazy_monkey_sm.add_state(lazy_monkey_attacking);
    lazy_monkey_sm.add_state(lazy_monkey_hit);
    lazy_monkey_sm.set_initial_state("Idle");

    return lazy_monkey_sm;
  }
}

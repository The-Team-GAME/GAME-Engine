//-----------------------------------------------------------------------------------
// file:    LevelTestState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a an empty state, useful for testing
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

class LevelTestState : public Engine::Scene
{
private:
  Engine::ECS::Entity player_entity;
  Engine::ECS::Entity lazy_monkey_entity;

  // Player dimensions
  const f32 player_idle_width = 64.0f;
  const f32 player_idle_height = 64.0f;
  const f32 player_attack_width = 96.0f;
  const f32 player_attack_height = 64.0f;

  // Lazy Monkey dimensions
  const f32 lazy_monkey_width = 64.0f;
  const f32 lazy_monkey_height = 64.0f;

public:
  void load() {
    Engine::Resources::open_level("levels/LevelTest.json");
  };

  void init() {
    // ========================================================================
    // Create Player with State Machine
    // ========================================================================

    // Define player states with animations

    // Player Idle Animation
    Components::StateAnimation player_idle_anim;
    player_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"));
    player_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 24, 24);
    player_idle_anim.animation = Components::Animation(24, 0.1f);
    player_idle_anim.scale = Math::Vec2D(player_idle_width * 2.0f, player_idle_height * 2.0f);

    Components::State player_idle;
    player_idle.name = "Idle";
    player_idle.on_enter = [this](Engine::ECS::Entity& entity) {
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

    // Player Walking Animation
    Components::StateAnimation player_walking_anim;
    player_walking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_walking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_walking_anim.animation = Components::Animation(8, 0.1f);
    player_walking_anim.scale = Math::Vec2D(player_idle_width * 2.0f, player_idle_height * 2.0f);

    Components::State player_walking;
    player_walking.name = "Walking";
    player_walking.on_enter = [this](Engine::ECS::Entity& entity) {
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

    // Player Jumping Animation (using run animation)
    Components::StateAnimation player_jumping_anim;
    player_jumping_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_jumping_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_jumping_anim.animation = Components::Animation(8, 0.1f);
    player_jumping_anim.scale = Math::Vec2D(player_idle_width * 2.0f, player_idle_height * 2.0f);

    Components::State player_jumping;
    player_jumping.name = "Jumping";
    player_jumping.on_enter = [this](Engine::ECS::Entity& entity) {
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

    // Player Attacking Animation
    Components::StateAnimation player_attacking_anim;
    player_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_threehit_spritesheet.png"));
    player_attacking_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 19, 6);
    player_attacking_anim.animation = Components::Animation(6, 0.05f);
    player_attacking_anim.scale = Math::Vec2D(player_attack_width * 2.0f, player_attack_height * 2.0f);

    Components::State player_attacking;
    player_attacking.name = "Attacking";
    player_attacking.on_enter = [this](Engine::ECS::Entity& entity) {
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
        float hitbox_height = player_attack_height * 2.0f;

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
    player_attacking.on_update = [this](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* animation = entity.get_component<Components::Animation>();

      // Transition to idle when attack animation completes
      if (animation && animation->has_ended()) {
        state_machine->change_state("Idle");
      }
      };
    player_attacking.on_exit = nullptr;
    player_attacking.animation = player_attacking_anim;

    // Create player state machine
    Components::StateMachine player_sm;
    player_sm.add_state(player_idle);
    player_sm.add_state(player_walking);
    player_sm.add_state(player_jumping);
    player_sm.add_state(player_attacking);
    player_sm.set_initial_state("Idle");

    // Create player entity
    player_entity = Engine::ECS::create_entity()
      .add_component<Components::Tag<"Player">>()
      .add_component<Components::Transform2D>(400.0f, 300.0f, 0.0f, player_idle_width * 2.0f, player_idle_height * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"))
      .add_component<Components::TextureAtlas>(64, 64, 1, 24, 24)
      .add_component<Components::Animation>(24, 0.1f)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Flip>(1.0f, 1.0f) // Start facing right
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(player_idle_width * 2.0f, player_idle_height * 2.0f, false)
      .add_component<Components::StateMachine>(player_sm)
      .remove_from_layer("default")
      .add_to_layer("player");

    // ========================================================================
    // Create Lazy Monkey with State Machine
    // ========================================================================

    // Define lazy_monkey states with animations

    // Lazy Monkey Idle Animation
    Components::StateAnimation lazy_monkey_idle_anim;
    lazy_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"));
    lazy_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    lazy_monkey_idle_anim.animation = Components::Animation(5, 0.15f);
    lazy_monkey_idle_anim.scale = Math::Vec2D(lazy_monkey_width * 2.0f, lazy_monkey_height * 2.0f);

    Components::State lazy_monkey_idle;
    lazy_monkey_idle.name = "Idle";
    lazy_monkey_idle.on_enter = [](Engine::ECS::Entity& entity) {
      std::cout << "Lazy Monkey: Entered Idle state" << std::endl;
      };
    lazy_monkey_idle.on_update = [this](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();

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

    // Lazy Monkey Chasing Animation
    Components::StateAnimation lazy_monkey_chasing_anim;
    lazy_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Walk_Spritesheet.png"));
    lazy_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    lazy_monkey_chasing_anim.animation = Components::Animation(6, 0.1f);
    lazy_monkey_chasing_anim.scale = Math::Vec2D(lazy_monkey_width * 2.0f, lazy_monkey_height * 2.0f);

    Components::State lazy_monkey_chasing;
    lazy_monkey_chasing.name = "Chasing";
    lazy_monkey_chasing.on_enter = [](Engine::ECS::Entity& entity) {
      std::cout << "Lazy Monkey: Entered Chasing state" << std::endl;
      };
    lazy_monkey_chasing.on_update = [this](Engine::ECS::Entity& entity) {
      auto* state_machine = entity.get_component<Components::StateMachine>();
      auto* rigid = entity.get_component<Components::RigidBody>();
      auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();

      // Attack cooldown tracking (in seconds)
      static float last_attack_time = 0.0f;
      const float attack_cooldown = 2.0f; // 2 seconds between attacks
      float current_time = Engine::Time::get_time();

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
              flip->value.x = (dx > 0 ? 1.0f : -1.0f);
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

    // Lazy Monkey Attacking Animation
    Components::StateAnimation lazy_monkey_attacking_anim;
    lazy_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Attack_Spritesheet.png"));
    lazy_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 4, 4);
    lazy_monkey_attacking_anim.animation = Components::Animation({ 0.2f, 0.1f, 0.1f, 0.5f }); // Custom frame durations
    lazy_monkey_attacking_anim.scale = Math::Vec2D(lazy_monkey_width * 2.0f, lazy_monkey_height * 2.0f);

    Components::State lazy_monkey_attacking;
    lazy_monkey_attacking.name = "Attacking";
    lazy_monkey_attacking.on_enter = [](Engine::ECS::Entity& entity) {
      auto* rigid = entity.get_component<Components::RigidBody>();
      auto* flip = entity.get_component<Components::Flip>();

      if (rigid && flip) {
        // Throw monkey forward based on facing direction
        float leap_speed = 400.f;
        rigid->velocity.x = (flip->value.x > 0 ? leap_speed : -leap_speed);
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

    // Lazy Monkey Hit Animation
    Components::StateAnimation lazy_monkey_hit_anim;
    lazy_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"));
    lazy_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 3, 3);
    lazy_monkey_hit_anim.animation = Components::Animation(3, 0.1f);
    lazy_monkey_hit_anim.scale = Math::Vec2D(lazy_monkey_width * 2.0f, lazy_monkey_height * 2.0f);

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

    // Create lazy_monkey state machine
    Components::StateMachine lazy_monkey_sm;
    lazy_monkey_sm.add_state(lazy_monkey_idle);
    lazy_monkey_sm.add_state(lazy_monkey_chasing);
    lazy_monkey_sm.add_state(lazy_monkey_attacking);
    lazy_monkey_sm.add_state(lazy_monkey_hit);
    lazy_monkey_sm.set_initial_state("Idle");

    // Create lazy_monkey entity
    lazy_monkey_entity = Engine::ECS::create_entity()
      .add_component<Components::Tag<"Lazy Monkey">>()
      .add_component<Components::Transform2D>(600.0f, 300.0f, 0.0f, lazy_monkey_width * 2.0f, lazy_monkey_height * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"))
      .add_component<Components::TextureAtlas>(64, 64, 1, 5, 5)
      .add_component<Components::Animation>(5, 0.15f)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Flip>(1.0f, 1.0f) // Start facing right
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(lazy_monkey_width * 2.0f, lazy_monkey_height * 2.0f, false)
      .add_component<Components::EnemyHealth>(100, 100)
      .add_component<Components::StateMachine>(lazy_monkey_sm)
      .remove_from_layer("default")
      .add_to_layer("enemy")
      .add_to_layer("damage_player");

    std::cout << "\n=== State Machine Test Level Loaded ===" << std::endl;
    std::cout << "Controls (Sidescrolling Platform Game):" << std::endl;
    std::cout << "  A/D - Move left/right" << std::endl;
    std::cout << "  SPACE - Jump" << std::endl;
    std::cout << "  F - Attack (damages lazy_monkey if close)" << std::endl;
    std::cout << "\nPlayer States: Idle (Blue), Walking (Green), Jumping (Cyan), Attacking (Red)" << std::endl;
    std::cout << "Lazy Monkey States: Idle (Yellow), Chasing (Orange), Attacking (Red), Hit (White)" << std::endl;
    std::cout << "Note: Gravity is active - entities will fall!" << std::endl;
    std::cout << "========================================\n" << std::endl;
  };

  void update() {};

  void draw() {
    Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });
  };

  void free() {};

  void unload() {
    // destroy all entities
    Engine::ECS::destroy_all_entities();

  };
};
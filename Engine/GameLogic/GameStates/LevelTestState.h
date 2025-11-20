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
  Engine::ECS::Entity enemy_entity;

public:
  void load() {
    Engine::Resources::open_level("levels/LevelTest.json");
  };

  void init() {
    // ========================================================================
    // Create Player with State Machine
    // ========================================================================

    // Define player states
    Components::State player_idle(
      "Idle",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* rigid = entity.get_component<Components::RigidBody>();
        if (rigid) rigid->velocity.x = 0.f;

        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(blue); // Blue when idle

        std::cout << "Player: Entered Idle state" << std::endl;
      },
      // on_update
      [](Engine::ECS::Entity& entity) {
        auto* state_machine = entity.get_component<Components::StateMachine>();

        // Check for horizontal movement input (A/D only for sidescrolling)
        if (Engine::Input::is_held(Engine::KeyCode::A) ||
          Engine::Input::is_held(Engine::KeyCode::D))
        {
          state_machine->change_state("Walking");
        }

        // Check for attack input
        if (Engine::Input::is_pressed(Engine::KeyCode::F))
        {
          state_machine->change_state("Attacking");
        }
      },
      // on_exit
      nullptr
    );

    Components::State player_walking(
      "Walking",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(0.0f, 1.0f, 0.0f, 1.0f); // Green when walking

        std::cout << "Player: Entered Walking state" << std::endl;
      },
      // on_update
      [](Engine::ECS::Entity& entity) {
        auto* rigid = entity.get_component<Components::RigidBody>();
        auto* state_machine = entity.get_component<Components::StateMachine>();

        float speed = 200.f;
        rigid->velocity.x = 0.f;
        // Don't touch velocity.y - let gravity handle it

        // Horizontal movement only (sidescrolling)
        if (Engine::Input::is_held(Engine::KeyCode::A)) {
          rigid->velocity.x = -speed;
        }
        if (Engine::Input::is_held(Engine::KeyCode::D)) {
          rigid->velocity.x = speed;
        }

        // Return to idle if no horizontal input
        if (!Engine::Input::is_held(Engine::KeyCode::A) &&
          !Engine::Input::is_held(Engine::KeyCode::D))
        {
          state_machine->change_state("Idle");
        }

        // Can attack while moving
        if (Engine::Input::is_pressed(Engine::KeyCode::F))
        {
          state_machine->change_state("Attacking");
        }
      },
      // on_exit
      [](Engine::ECS::Entity& entity) {
        auto* rigid = entity.get_component<Components::RigidBody>();
        if (rigid) {
          rigid->velocity.x = 0.f;
          // Don't reset velocity.y - let gravity/physics handle it
        }
      }
    );

    Components::State player_attacking(
      "Attacking",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* rigid = entity.get_component<Components::RigidBody>();
        if (rigid) {
          rigid->velocity.x = 0.f;
          // Don't reset velocity.y - let gravity/physics handle it
        }

        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(red); // Red when attacking

        std::cout << "Player: Entered Attacking state" << std::endl;
      },
      // on_update
      [this](Engine::ECS::Entity& entity) {
        auto* state_machine = entity.get_component<Components::StateMachine>();

        // Simple timer: attack lasts ~30 frames
        static int attack_timer = 0;
        attack_timer++;

        if (attack_timer >= 30) {
          attack_timer = 0;

          // Check if enemy is nearby and damage it
          if (enemy_entity.is_valid()) {
            auto* player_transform = entity.get_component<Components::Transform2D>();
            auto* enemy_transform = enemy_entity.get_component<Components::Transform2D>();

            if (player_transform && enemy_transform) {
              float dx = player_transform->position.x - enemy_transform->position.x;
              float dy = player_transform->position.y - enemy_transform->position.y;
              float distance = std::sqrt(dx * dx + dy * dy);

              if (distance < 150.f) {
                auto* enemy_sm = enemy_entity.get_component<Components::StateMachine>();
                if (enemy_sm) {
                  enemy_sm->change_state("Hit");
                }
              }
            }
          }

          state_machine->change_state("Idle");
        }
      },
      // on_exit
      nullptr
    );

    // Create player state machine
    Components::StateMachine player_sm;
    player_sm.add_state(player_idle);
    player_sm.add_state(player_walking);
    player_sm.add_state(player_attacking);
    player_sm.set_initial_state("Idle");

    // Create player entity
    player_entity = Engine::ECS::create_entity()
      .add_component<Components::Tag<"Player">>()
      .add_component<Components::Transform2D>(400.0f, 300.0f, 0.0f, 50.0f, 50.0f)
      .add_component<Components::Color>(blue)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(50.0f, 50.0f, false)
      .add_component<Components::StateMachine>(player_sm)
      .remove_from_layer("default")
      .add_to_layer("player");

    // ========================================================================
    // Create Enemy with State Machine
    // ========================================================================

    // Define enemy states
    Components::State enemy_idle(
      "Idle",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow when idle

        std::cout << "Enemy: Entered Idle state" << std::endl;
      },
      // on_update
      [this](Engine::ECS::Entity& entity) {
        auto* state_machine = entity.get_component<Components::StateMachine>();

        // Check if player is nearby
        if (player_entity.is_valid()) {
          auto* enemy_transform = entity.get_component<Components::Transform2D>();
          auto* player_transform = player_entity.get_component<Components::Transform2D>();

          if (enemy_transform && player_transform) {
            float dx = player_transform->position.x - enemy_transform->position.x;
            float dy = player_transform->position.y - enemy_transform->position.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Start chasing if player is within 300 units
            if (distance < 300.f) {
              state_machine->change_state("Chasing");
            }
          }
        }
      },
      // on_exit
      nullptr
    );

    Components::State enemy_chasing(
      "Chasing",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(1.0f, 0.65f, 0.0f, 1.0f); // Orange when chasing

        std::cout << "Enemy: Entered Chasing state" << std::endl;
      },
      // on_update
      [this](Engine::ECS::Entity& entity) {
        auto* state_machine = entity.get_component<Components::StateMachine>();
        auto* rigid = entity.get_component<Components::RigidBody>();
        auto* enemy_transform = entity.get_component<Components::Transform2D>();

        if (player_entity.is_valid() && rigid && enemy_transform) {
          auto* player_transform = player_entity.get_component<Components::Transform2D>();

          if (player_transform) {
            float dx = player_transform->position.x - enemy_transform->position.x;
            float distance = std::abs(dx); // Only horizontal distance for sidescrolling

            // Chase player horizontally
            if (distance > 10.f) {
              float speed = 100.f;
              rigid->velocity.x = (dx > 0 ? speed : -speed);
              // Don't modify velocity.y - let gravity handle it
            }
            else {
              rigid->velocity.x = 0.f;
            }

            // Return to idle if player is far away
            if (distance > 400.f) {
              state_machine->change_state("Idle");
            }

            // Attack if close enough
            if (distance < 80.f) {
              state_machine->change_state("Attacking");
            }
          }
        }
      },
      // on_exit
      [](Engine::ECS::Entity& entity) {
        auto* rigid = entity.get_component<Components::RigidBody>();
        if (rigid) {
          rigid->velocity.x = 0.f;
          // Don't reset velocity.y - let gravity/physics handle it
        }
      }
    );

    Components::State enemy_attacking(
      "Attacking",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* rigid = entity.get_component<Components::RigidBody>();
        if (rigid) {
          rigid->velocity.x = 0.f;
          // Don't reset velocity.y - let gravity/physics handle it
        }

        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(red); // Red when attacking

        std::cout << "Enemy: Entered Attacking state" << std::endl;
      },
      // on_update
      [](Engine::ECS::Entity& entity) {
        auto* state_machine = entity.get_component<Components::StateMachine>();

        // Simple timer: attack lasts ~40 frames
        static int attack_timer = 0;
        attack_timer++;

        if (attack_timer >= 40) {
          attack_timer = 0;
          state_machine->change_state("Chasing");
        }
      },
      // on_exit
      nullptr
    );

    Components::State enemy_hit(
      "Hit",
      // on_enter
      [](Engine::ECS::Entity& entity) {
        auto* color = entity.get_component<Components::Color>();
        if (color) *color = Components::Color(white); // White when hit (flash)

        auto* rigid = entity.get_component<Components::RigidBody>();
        if (rigid) {
          rigid->velocity.x = 0.f;
          // Don't reset velocity.y - let gravity/physics handle it
        }

        std::cout << "Enemy: Got hit!" << std::endl;
      },
      // on_update
      [](Engine::ECS::Entity& entity) {
        auto* state_machine = entity.get_component<Components::StateMachine>();

        // Flash briefly
        static int hit_timer = 0;
        hit_timer++;

        if (hit_timer >= 15) {
          hit_timer = 0;
          state_machine->change_state("Idle");
        }
      },
      // on_exit
      nullptr
    );

    // Create enemy state machine
    Components::StateMachine enemy_sm;
    enemy_sm.add_state(enemy_idle);
    enemy_sm.add_state(enemy_chasing);
    enemy_sm.add_state(enemy_attacking);
    enemy_sm.add_state(enemy_hit);
    enemy_sm.set_initial_state("Idle");

    // Create enemy entity
    enemy_entity = Engine::ECS::create_entity()
      .add_component<Components::Tag<"Enemy">>()
      .add_component<Components::Transform2D>(600.0f, 300.0f, 0.0f, 50.0f, 50.0f)
      .add_component<Components::Color>(1.0f, 1.0f, 0.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::get_rect_texture())
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(50.0f, 50.0f, false)
      .add_component<Components::StateMachine>(enemy_sm)
      .remove_from_layer("default")
      .add_to_layer("enemy");

    std::cout << "\n=== State Machine Test Level Loaded ===" << std::endl;
    std::cout << "Controls (Sidescrolling Platform Game):" << std::endl;
    std::cout << "  A/D - Move left/right" << std::endl;
    std::cout << "  SPACE - Attack (damages enemy if close)" << std::endl;
    std::cout << "\nPlayer States: Idle (Blue), Walking (Green), Attacking (Red)" << std::endl;
    std::cout << "Enemy States: Idle (Yellow), Chasing (Orange), Attacking (Red), Hit (White)" << std::endl;
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
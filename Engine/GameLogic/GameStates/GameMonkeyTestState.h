//-----------------------------------------------------------------------------------
// file:    GameMonkeyTestState.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file defines a an empty state, useful for testing
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../States/PlayerStateMachine.h"
#include "../States/GamerMonkeyStateMachine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"
#include "../Components/HealthComponents.h"

class GameMonkeyTestState : public Engine::Scene
{
private:
  Engine::ECS::Entity player_entity;
  Engine::ECS::Entity Gamer_monkey_entity;

public:
  void load() {
    Engine::Resources::open_level("levels/MonkeyTester.json");
  };

  void init() {
    // ========================================================================
    // Create Player Entity
    // ========================================================================
    player_entity = Engine::ECS::create_entity()
      .add_component<Components::Player>()
      .add_component<Components::Transform2D>(400.0f, 300.0f, 0.0f, PlayerStates::IDLE_WIDTH * 2.0f, PlayerStates::IDLE_HEIGHT * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"))
      .add_component<Components::TextureAtlas>(64, 64, 1, 24, 24)
      .add_component<Components::Animation>(24, 0.1f)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Flip>(1.0f, 1.0f) // Start facing right
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(PlayerStates::IDLE_WIDTH * 2.0f, PlayerStates::IDLE_HEIGHT * 2.0f, false)
      .add_component<Components::StateMachine>(PlayerStates::create_state_machine())
      .add_component<Components::Heartrate>()
      .add_component<Components::Trail>(0.05f, 0.0f, 0.8f, 0.0f) // Interval, Lifetime, StartAlpha, EndAlpha 
      .add_component<Components::Distortion>(0.0f, 0.0f, 0.0f) // Intensity, Speed, MeshIntensity
      .add_component<Components::HeartHUDSprite>()
      .remove_from_layer("default")
      .add_to_layer("player");

    // ========================================================================
    // Create Camera Entity
    // ========================================================================
    Engine::ECS::create_entity()
      .add_component<Components::Camera>("Main Camera", true)
      .add_component<Components::Transform2D>(0.0f, -400.0f, 0.0f, 1.0f, 1.0f) // Relative to parent
      .add_component<Components::Parent>(player_entity.get_id());

    // ========================================================================
    // Create Gamer Monkey Entity
    // ========================================================================
    Gamer_monkey_entity = Engine::ECS::create_entity()
      .add_component<Components::Enemy>()
      .add_component<Components::GamerMonkey>()
      .add_component<Components::Transform2D>(800.0f, 400.0f, 0.0f, GamerMonkeyStates::WIDTH, GamerMonkeyStates::HEIGHT)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/idle.png"))
      .add_component<Components::TextureAtlas>(64, 64, 1, 5, 5)
      .add_component<Components::Animation>(4, 0.15f)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Flip>(1.0f, 1.0f) // Start facing right
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(GamerMonkeyStates::WIDTH, GamerMonkeyStates::HEIGHT, false)
      .add_component<Components::EnemyHealth>(400, 400)
      .add_component<Components::Attack>(10, Math::Vec2D(900.0f, -300.0f), true, 1.0f)
      .add_component<Components::EnemyHealthBarRect>()
      .add_component<Components::StateMachine>(GamerMonkeyStates::create_state_machine())
      .remove_from_layer("default")
      .add_to_layer("enemy");
    //.add_to_layer("damage_player");


    // Sync RigidBody mass/friction with gameplay data so knockback doesn't cause long glides
    if (auto* player_rb = player_entity.get_component<Components::RigidBody>()) {
      if (auto* player_comp = player_entity.get_component<Components::Player>()) {
        player_rb->mass = player_comp->mass;
      }
      player_rb->kinetic_coefficient = 3.0f; // Stronger ground friction for faster stopping
    }

    if (auto* gm_rb = Gamer_monkey_entity.get_component<Components::RigidBody>()) {
      if (auto* gm_comp = Gamer_monkey_entity.get_component<Components::GamerMonkey>()) {
        gm_rb->mass = gm_comp->mass;
      }
      gm_rb->kinetic_coefficient = 3.0f; // Match player friction behaviour
    }

    // Collider friction value (used by some tools; 1 = no loss, 0 = full loss)
    if (auto* player_col = player_entity.get_component<Components::BoxCollider>()) {
      player_col->kinetic_friction = 0.8f;
    }
    if (auto* gm_col = Gamer_monkey_entity.get_component<Components::BoxCollider>()) {
      gm_col->kinetic_friction = 0.8f;
    }

    Log::info("\n=== State Machine Test Level Loaded ===");
    Log::info("Controls (Sidescrolling Platform Game):");
    Log::info("  A/D - Move left/right");
    Log::info("  SPACE - Jump");
    Log::info("  F - Attack (damages Gamer_monkey if close)");
    Log::info("\nPlayer States: Idle (Blue), Walking (Green), Jumping (Cyan), Attacking (Red)");
    Log::info("Lazy Monkey States: Idle (Yellow), Chasing (Orange), Attacking (Red), Hit (White)");
    Log::info("Note: Gravity is active - entities will fall!");
    Log::info("========================================\n");
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
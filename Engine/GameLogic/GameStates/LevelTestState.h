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
#include "../States/PlayerStateMachine.h"
#include "../States/LazyMonkeyStateMachine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"

#include "../Components/HealthComponents.h"
#include "../Systems/HealthSystem.h"
#include "../../Engine/Systems/ParticleSystem.h"

class LevelTestState : public Engine::Scene
{
private:
  Engine::ECS::Entity player_entity;
  Engine::ECS::Entity lazy_monkey_entity;

public:
  void load() {
    Engine::Resources::open_level("levels/LevelTest.json");

    // Engine::Resources::load_prefab("prefabs/player.json").instantiate().get_component<Components::Transform2D>()->position.x = 100;
    // Engine::Resources::load_prefab("prefabs/lazy_monkey.json").instantiate().get_component<Components::Transform2D>()->position.x = 900;
  };

  void init() {

    //Engine::ECS::add_system<Engine::Systems::EnemyHealthBarSystem>();
    Engine::ECS::add_system<Engine::Systems::EnemyHealthBarRectSystem>();
    Engine::ECS::add_system<Engine::Systems::HeartHUDSpriteSystem>();
    Engine::ECS::add_system<Engine::Systems::ParticleSystem>();

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
      .remove_from_layer("default")
      .add_to_layer("player");

    player_entity.add_component<Components::HeartHUDSprite>();
    if (auto* hud = player_entity.get_component<Components::HeartHUDSprite>()) {
      // Textures
      hud->tex_back = Engine::Resources::load_texture("images/Heartratebar/BarBack.png");
      hud->tex_solid = Engine::Resources::load_texture("images/Heartratebar/WhiteSolidForFill.png");
      hud->tex_head = Engine::Resources::load_texture("images/Heartratebar/HeadBar.png");
      hud->tex_white = Engine::Resources::load_texture("images/Heartratebar/WhiteBar.png");
    }

    // ========================================================================
    // Create Lazy Monkey Entity
    // ========================================================================
    lazy_monkey_entity = Engine::ECS::create_entity()
      .add_component<Components::Enemy>()
      .add_component<Components::LazyMonkey>()
      .add_component<Components::Transform2D>(600.0f, 300.0f, 0.0f, LazyMonkeyStates::WIDTH * 2.0f, LazyMonkeyStates::HEIGHT * 2.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"))
      .add_component<Components::TextureAtlas>(64, 64, 1, 5, 5)
      .add_component<Components::Animation>(5, 0.15f)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Flip>(1.0f, 1.0f) // Start facing right
      .add_component<Components::RigidBody>(Components::RigidBody(false, true))
      .add_component<Components::BoxCollider>(LazyMonkeyStates::WIDTH * 2.0f, LazyMonkeyStates::HEIGHT * 2.0f, false)
      .add_component<Components::EnemyHealth>(400, 400)
      .add_component<Components::EnemyHealthBarRect>()
      //.add_component<Components::EnemyHealthBar>()
      .add_component<Components::StateMachine>(LazyMonkeyStates::create_state_machine())
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

  void update() {

  
  };

  void draw() {
    Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });
  };

  void free() {};

  void unload() {
    // destroy all entities
    Engine::ECS::destroy_all_entities();

  };
};
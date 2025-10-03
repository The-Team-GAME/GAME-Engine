//-----------------------------------------------------------------------------------
// file:    EcsState.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   This file defines the EcsState which demonstrates the ECS architecture.
//          Shows entity creation, component management, system integration, entity
//          cloning, and event-driven gameplay with health and damage systems.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "ObjectFactory.h"
#include "MovementSystem.h"
#include "RenderSystem.h" 
#include "BackgroundSystem.h"
#include "PlayerControllerSystem.h"
#include "PhysicsSystem.h"
#include "GravitySystem.h"
#include "CollisionSystem.h"
#include "TypeDef.h"

class PhysicsTestState : public Engine::GameState<PhysicsTestState>
{
private:
  s32 enemy_prefab = -1;

  // Add font IDs
  s32 title_font_id = -1;
  s32 info_font_id = -1;

public:
  void Load() {
    // Load fonts
    title_font_id = Engine::RESOURCES.load_font("physics_title_font", "Assets/fonts/PlayfairDisplay-VariableFont_wght.ttf", 82);
    info_font_id = Engine::RESOURCES.load_font("physics_info_font", "Assets/fonts/Roboto-Regular.ttf", 48);
  };

  void Init()
  {
    auto player = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(200.0f, 200.0f)
      .add_component<Components::Color>(0.2f, 1.0f, 0.2f, 1.0f)
      .add_component<Components::Size>(20.0f, 20.0f)
      .add_component<Components::Gravity>()
      .add_component<Components::RigidBody>(1.0f) // mass=1.0, 
      .add_component<Components::Collider>(20.0f, 20.0f, false) // x, y, isStatic
      .add_component<Components::PlayerController>();

    auto platform1 = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(500.0f, 300.0f)
      .add_component<Components::Color>(1.0f, 0.2f, 0.2f, 1.0f)
      .add_component<Components::Size>(400.0f, 100.0f)
      .add_component<Components::RigidBody>(1.0f) // mass=1.0,
      .add_component<Components::Gravity>()
      .add_component<Components::Collider>(400.0f, 100.0f, true); // x, y, isStatic

    auto platform2 = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(500.0f, 500.0f)
      .add_component<Components::Color>(1.0f, 0.2f, 0.2f, 1.0f)
      .add_component<Components::Size>(4000.0f, 100.0f)
      .add_component<Components::RigidBody>(10000000.0f) // mass=1.0,
      .add_component<Components::Collider>(4000.0f, 100.0f, true); // x, y, isStatic


    auto background = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD.add_system<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.add_system<GravitySystem>();

    Engine::WORLD.add_system<PlayerControllerSystem>();


    Engine::WORLD.add_system<MovementSystem>();
    Engine::WORLD.add_system<PhysicsSystem>();
    Engine::WORLD.add_system<CollisionSystem>();

    Engine::WORLD.add_system<RectRenderSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();
  };

  void Update() {

  };

  void Draw()
  {
    // Draw title text
    if (title_font_id != -1) {
      Engine::GRAPHICS.draw_text(
        title_font_id,
        "Physics Test Scene",
        50.0f, 50.0f,
        1.0f,
        { 1.0f, 0.9f, 0.3f, 1.0f } // Gold color
      );
    }

    // Draw control info
    if (info_font_id != -1) {
      Engine::GRAPHICS.draw_text(
        info_font_id,
        "WASD: Move Player | SPACE: Jump",
        50.0f, 140.0f,
        0.6f,
        { 1.0f, 1.0f, 1.0f, 1.0f } // White
      );
    }
  };

  void Free() {
    Engine::FACTORY.unregisterObject(enemy_prefab);
    Engine::WORLD.destroy_entities_with<Components::Tag<"GameObjectState">>();
    // Remove systems
    Engine::WORLD.remove_system<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.remove_system<GravitySystem>();
    Engine::WORLD.remove_system<PlayerControllerSystem>();
    Engine::WORLD.remove_system<RectRenderSystem>();
    Engine::WORLD.remove_system<MovementSystem>();
    Engine::WORLD.remove_system<PhysicsSystem>();
    Engine::WORLD.remove_system<CollisionSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
  };

  void Unload() {
    Engine::FACTORY.unregisterObject(enemy_prefab);

    // Unload fonts
    Engine::RESOURCES.unload_font("physics_title_font");
    Engine::RESOURCES.unload_font("physics_info_font");
  }
};
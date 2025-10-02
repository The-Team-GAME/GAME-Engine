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

class PhysicsTestScene : public Engine::GameState<PhysicsTestScene>
{
private:
  int enemyPrefab = -1;
public:
  void Load() {};

  void Init()
  {
    // Load the enemy prefab from a JSON file
    enemyPrefab = Engine::FACTORY.registerObject("Assets/objects/enemy.json");


    // Create two enemy instances from the prefab
    auto enemy1 = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(200.0f, 200.0f)
      .add_component<Components::Color>(0.2f, 1.0f, 0.2f, 1.0f)
      .add_component<Components::Size>(20.0f, 20.0f)
      .add_component<Components::Gravity>()
      .add_component<Components::RigidBody>(1.0f) // mass=1.0, 
      .add_component<Components::Collider>(20.0f, 20.0f, false) // x, y, isStatic
      .add_component<Components::PlayerController>();

    auto enemy3 = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(500.0f, 300.0f)
      .add_component<Components::Color>(1.0f, 0.2f, 0.2f, 1.0f)
      .add_component<Components::Size>(100.0f, 50.0f)
      .add_component<Components::RigidBody>(1.0f) // mass=1.0,
      .add_component<Components::Gravity>()
      .add_component<Components::Collider>(100.0f, 50.0f, true); // x, y, isStatic


    // Create two enemy instances from the prefab
    auto enemy2 = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"Floor">>()
      .add_component<Components::Position>(500.0f, 500.0f)
      .add_component<Components::Color>(1.0f, 0.2f, 0.2f, 1.0f)
      .add_component<Components::Size>(4000.0f, 50.0f)
      .add_component<Components::RigidBody>(10000000.0f) // mass=1.0,
      .add_component<Components::Collider>(4000.0f, 50.0f, true); // x, y, isStatic





    // You can modify components like this too
    //enemy2.getComponent<Components::Position>()->x = 300.0f;

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
  };

  void Free() {
    Engine::FACTORY.unregisterObject(enemyPrefab);
    Engine::WORLD.destroy_entities_with<Components::Tag<"GameObjectState">>();
    Engine::WORLD.destroy_entities_with<Components::Tag<"Floor">>();
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
    Engine::FACTORY.unregisterObject(enemyPrefab);
  }
};
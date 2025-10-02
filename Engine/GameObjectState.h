//-----------------------------------------------------------------------------------
// file:    GameObjectState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the GameObjectState which demonstrates the ObjectFactory
//          prefab system. Shows how to load prefabs from JSON files and instantiate
//          multiple entities with different component configurations.
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

class GameObjectState : public Engine::GameState<GameObjectState>
{
private:
  s32 enemyPrefab = -1;
public:
  void Load() {};

  void Init()
  {
    // Load the enemy prefab from a JSON file
    enemyPrefab = Engine::FACTORY.registerObject("Assets/objects/enemy.json");

    // Create two enemy instances from the prefab
    auto enemy1 = Engine::FACTORY.createObject(enemyPrefab)
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(100.0f, 100.0f); // Position the first enemy left
    auto enemy2 = Engine::FACTORY.createObject(enemyPrefab)
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(300.0f, 100.0f) // Position the second enemy right
      .add_component<Components::Scale>(2.0f, 2.0f); // Make it 2 times bigger

    // You can modify components like this too
    //enemy2.get_component<Components::Position>()->x = 300.0f;

    auto background = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD.add_system<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.add_system<MovementSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();
  };

  void Update() {};

  void Draw()
  {
  };

  void Free() {
    Engine::FACTORY.unregisterObject(enemyPrefab);
    Engine::WORLD.destroy_entities_with<Components::Tag<"GameObjectState">>();

    // Remove systems
    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<MovementSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
  };

  void Unload() {};
};
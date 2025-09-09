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
  int enemyPrefab;
public:
  void Load() {};

  void Init()
  {
    // Load the enemy prefab from a JSON file
    enemyPrefab = Engine::FACTORY->registerObject("Assets/objects/enemy.json");

    // Create two enemy instances from the prefab
    auto enemy1 = Engine::FACTORY->createObject(enemyPrefab)
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Position>(100.0f, 100.0f); // Position the first enemy left
    auto enemy2 = Engine::FACTORY->createObject(enemyPrefab)
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Position>(300.0f, 100.0f); // Position the second enemy right

    // You can modify components like this too
    //enemy2.getComponent<Components::Position>()->x = 300.0f;

    auto background = Engine::WORLD->createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD->addSystem<BackgroundSystem>(); // Background system should be first
    Engine::WORLD->addSystem<MovementSystem>();
    Engine::WORLD->addSystem<GameObjectRenderSystem>();
  };

  void Update() {};

  void Draw()
  {
  };

  void Free() {
    Engine::FACTORY->unregisterObject(enemyPrefab);
    Engine::WORLD->destroyEntitiesWith<Components::Tag<"GameObjectState">>();
  };

  void Unload() {};
};
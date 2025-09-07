#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "ObjectFactory.h"
#include "MovementSystem.h"
#include "RenderSystem.h"

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
      .addComponent<Components::Tag<"GameObjectState">>();
    auto enemy2 = Engine::FACTORY->createObject(enemyPrefab)
      .addComponent<Components::Tag<"GameObjectState">>();

    // Position them differently
    enemy1.getComponent<Components::Position>()->x = 100.0f;
    enemy2.getComponent<Components::Position>()->x = 300.0f;

    // Add systems to the world
    Engine::WORLD->addSystem<MovementSystem>();
    Engine::WORLD->addSystem<GameObjectRenderSystem>();
  };

  void Update() {};

  void Draw()
  {
    Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
  };

  void Free() {
    Engine::FACTORY->unregisterObject(enemyPrefab);
    Engine::WORLD->destroyEntitiesWith<Components::Tag<"GameObjectState">>();
  };

  void Unload() {};
};
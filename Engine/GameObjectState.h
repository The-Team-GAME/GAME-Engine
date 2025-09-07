#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "ObjectFactory.h"
#include "MovementSystem.h"
#include "RenderSystem.h"

class GameObjectState : public Engine::GameState<GameObjectState>
{
private:

public:
  void Load() {};

  void Init()
  {
    int enemyPrefab = Engine::FACTORY->fromJsonFile("Assets/objects/enemy.json");

    auto enemy1 = Engine::FACTORY->createObject(enemyPrefab);
    auto enemy2 = Engine::FACTORY->createObject(enemyPrefab);

    enemy1.getComponent<Components::Position>()->x = 100.0f;
    enemy2.getComponent<Components::Position>()->x = 300.0f;

    Engine::WORLD->addSystem<MovementSystem>();
    Engine::WORLD->addSystem<GameObjectRenderSystem>();
  };

  void Update() {};

  void Draw()
  {
    Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
  };

  void Free() {};

  void Unload() {};
};
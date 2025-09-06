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

    Engine::FACTORY->createObject(enemyPrefab);


    Engine::WORLD->addSystem<MovementSystem>();
    Engine::WORLD->addSystem<GameObjectRenderSystem>();
  };

  void Update() {};

  void Draw()
  {
    Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
  };

  void Free() {};

  void Unload() {};
};
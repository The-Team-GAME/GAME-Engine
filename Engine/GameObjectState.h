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
    enemyPrefab = Engine::FACTORY->registerObject("Assets/objects/enemy.json");

    auto enemy1 = Engine::FACTORY->createObject(enemyPrefab).addComponent<Components::Tag<"GameObjectState">>();
    auto enemy2 = Engine::FACTORY->createObject(enemyPrefab).addComponent<Components::Tag<"GameObjectState">>();

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

  void Free() {
    Engine::FACTORY->unregisterObject(enemyPrefab);

    auto entities = Engine::WORLD->getEntitiesWith<Components::Tag<"GameObjectState">>();
    for (auto entityId : entities) {
      Engine::WORLD->destroyEntity(entityId);
    }
  };

  void Unload() {};
};
#pragma once

#include "StateManager.h" 
#include "Core.h"
#include "BackgroundSystem.h"
#include "RenderSystem.h"


// Used to test how well the engine handles many sprites
class DevManySpritesState : public Engine::GameState<DevManySpritesState>
{
private:
  int enemyPrefab;

public:
  void Load() {
    // Load the enemy prefab from a JSON file
    enemyPrefab = Engine::FACTORY->registerObject("Assets/objects/enemy.json");

    for (int i = 0; i < 1000; ++i) { // Create 1000 enemies
      float x = static_cast<float>(rand() % 800); // Random x position within window width
      float y = static_cast<float>(rand() % 600); // Random y position within window height
      Engine::FACTORY->createObject(enemyPrefab)
        .addComponent<Components::Tag<"GameObjectState">>()
        .addComponent<Components::Position>(x, y);
    }

    auto background = Engine::WORLD->createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD->addSystem<BackgroundSystem>(); // Background system should be first
    Engine::WORLD->addSystem<StaticSpriteRenderSystem>();
  }

  void Init() {}

  void Update() {}

  void Draw() {}

  void Free() {
    Engine::FACTORY->unregisterObject(enemyPrefab);
    Engine::WORLD->destroyEntitiesWith<Components::Tag<"GameObjectState">>();

    // Remove systems
    Engine::WORLD->removeSystem<BackgroundSystem>();
    Engine::WORLD->removeSystem<StaticSpriteRenderSystem>();
  }

  void Unload() {}
};
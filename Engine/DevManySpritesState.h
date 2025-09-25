#pragma once

#include "StateManager.h" 
#include "Core.h"
#include "BackgroundSystem.h"
#include "RenderSystem.h"
#include "RotationSystem.h"


// Used to test how well the engine handles many sprites
class DevManySpritesState : public Engine::GameState<DevManySpritesState>
{
private:
  int enemyPrefab = -1;
  int bananaPrefab = -1;

public:
  void Load() {
    // Load the enemy prefab from a JSON file
    enemyPrefab = Engine::FACTORY.registerObject("Assets/objects/enemy.json");
    bananaPrefab = Engine::FACTORY.registerObject("Assets/objects/banana.json");

    for (int i = 0; i < 2500; ++i) { // Create 1000 enemies
      float x = static_cast<float>(rand() % 800); // Random x position within window width
      float y = static_cast<float>(rand() % 600); // Random y position within window height
      Engine::FACTORY.createObject(bananaPrefab)
        .addComponent<Components::Tag<"DevManySpritesState">>()
        .addComponent<Components::Position>(x, y)
        .addComponent<Components::Rotation>(0.0f)
        .addComponent<Components::RotationSpeed>(1.0f); // Random rotation speed between 30 and 120 degrees per second
    }

    auto background = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"DevManySpritesState">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD.addSystem<BackgroundSystem>();
    Engine::WORLD.addSystem<RotationSystem>();
    Engine::WORLD.addSystem<StaticSpriteRenderSystem>();
  }

  void Init() {}

  void Update() {}

  void Draw() {}

  void Free() {
    Engine::FACTORY.unregisterObject(enemyPrefab);
    Engine::WORLD.destroyEntitiesWith<Components::Tag<"DevManySpritesState">>();

    // Remove systems
    Engine::WORLD.removeSystem<BackgroundSystem>();
    Engine::WORLD.removeSystem<RotationSystem>();
    Engine::WORLD.removeSystem<StaticSpriteRenderSystem>();
  }

  void Unload() {}
};
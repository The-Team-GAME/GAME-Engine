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
  s32 enemyPrefab = -1;
  s32 bananaPrefab = -1;

public:
  void Load() {
    // Load object prefabs through ResourceManager  
    enemyPrefab = Engine::RESOURCES.load_object_prefab("enemy", "Assets/objects/enemy.json");
    bananaPrefab = Engine::RESOURCES.load_object_prefab("banana", "Assets/objects/banana.json");

    for (int i = 0; i < 2500; ++i) { // Create 2500 bananas
      f32 x = static_cast<f32>(rand() % 1600); // Random x position within window width
      f32 y = static_cast<f32>(rand() % 900); // Random y position within window height
      Engine::FACTORY.createObject(bananaPrefab)
        .add_component<Components::Tag<"DevManySpritesState">>()
        .add_component<Components::Position>(x, y)
        .add_component<Components::Rotation>(0.0f)
        .add_component<Components::RotationSpeed>(45.0f);
    }

    auto background = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"DevManySpritesState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD.addSystem<BackgroundSystem>();
    Engine::WORLD.addSystem<RotationSystem>();
    Engine::WORLD.addSystem<StaticSpriteRenderSystem>();
  }

  void Init() {}

  void Update() {}

  void Draw() {}

  void Free() {
    // Unload through ResourceManager (new approach)
    Engine::RESOURCES.unload_object_prefab("enemy");
    Engine::RESOURCES.unload_object_prefab("banana");

    Engine::WORLD.destory_entities_with<Components::Tag<"DevManySpritesState">>();

    // Remove systems
    Engine::WORLD.removeSystem<BackgroundSystem>();
    Engine::WORLD.removeSystem<RotationSystem>();
    Engine::WORLD.removeSystem<StaticSpriteRenderSystem>();
  }

  void Unload() {}
};
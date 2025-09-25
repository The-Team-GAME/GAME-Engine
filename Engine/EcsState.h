#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "ObjectFactory.h"
#include "MovementSystem.h"
#include "RenderSystem.h"
#include "ComponentSelector.h"
#include "BackgroundSystem.h"
#include "PlayerControllerSystem.h"
#include "PhysicsSystem.h"
#include "CollisionSystem.h"

class EcsState : public Engine::GameState<EcsState>
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
    auto enemy1 = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Position>(100.0f, 100.0f)
			.addComponent<Components::Color>(0.2f,1.0f,0.2f,1.0f)
      .addComponent<Components::Size>(20.0f, 20.0f)
      .addComponent<Components::Velocity>(0.0f, 0.0f)
      .addComponent<Components::RigidBody>(1.0f, false) 
      .addComponent<Components::Collider>(20.0f, 20.0f, false)
      .addComponent<Components::PlayerController>();

    // Create two enemy instances from the prefab
    auto enemy2 = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Position>(100.0f, 100.0f)
      .addComponent<Components::Color>(1.0f, 0.2f, 0.2f, 1.0f)
			.addComponent<Components::Size>(50.0f, 50.0f)
      .addComponent<Components::Velocity>(0.0f, 0.0f)
      .addComponent<Components::RigidBody>(1.0f, false)
      .addComponent<Components::Collider>(50.0f, 50.0f, false);
      


    // You can modify components like this too
    //enemy2.getComponent<Components::Position>()->x = 300.0f;

    auto background = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD.addSystem<BackgroundSystem>(); // Background system should be first
		Engine::WORLD.addSystem<PlayerControllerSystem>();
    Engine::WORLD.addSystem<RectRenderSystem>();
	  Engine::WORLD.addSystem<PhysicsSystem>();
    Engine::WORLD.addSystem<MovementSystem>();
		Engine::WORLD.addSystem<CollisionSystem>();
    Engine::WORLD.addSystem<StaticSpriteRenderSystem>();
  };

  void Update() {
		
  };

  void Draw()
  {
    EditorUI::ComponentSelector::DrawAddComponentButtons(1); // Example entity ID
  };

  void Free() {
    Engine::FACTORY.unregisterObject(enemyPrefab);
    Engine::WORLD.destroyEntitiesWith<Components::Tag<"GameObjectState">>();

    // Remove systems
    Engine::WORLD.removeSystem<BackgroundSystem>();
    Engine::WORLD.removeSystem<PlayerControllerSystem>();
    Engine::WORLD.removeSystem<RectRenderSystem>();
    Engine::WORLD.removeSystem<PhysicsSystem>();
    Engine::WORLD.removeSystem<MovementSystem>();
    Engine::WORLD.removeSystem<CollisionSystem>();
    Engine::WORLD.removeSystem<StaticSpriteRenderSystem>();
  };

  void Unload() {};
};
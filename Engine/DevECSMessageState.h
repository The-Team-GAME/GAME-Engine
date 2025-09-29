#pragma once

#include "StateManager.h"
#include "EcsState.h"
#include "Graphics.h"
#include <iostream>
#include <glm/glm.hpp>
#include "Core.h"
#include "BasicComponents.h"
#include "MovementSystem.h"
#include "HealthSystem.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "InputManager.h"
#include "BasicEvents.h"
#include "DamageSystem.h"

class DevECSMessageState : public Engine::GameState<DevECSMessageState>
{
private:
  int enemyPrefab;
  ECS::Entity player;

public:
  void Load() {
    enemyPrefab = Engine::FACTORY.registerObject("Assets/objects/enemy.json");
  }

  void Init() {
    // Create player entity
    player = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"Player">>()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Position>(10.0f, 20.0f)
      .addComponent<Components::Velocity>(100.0f, 0.0f) // Make player move
			.addComponent<Components::RigidBody>(1.0f, false) // mass=1.0, not static
      .addComponent<Components::Health>(100, 100)
      .addComponent<Components::Size>(10.0f, 10.0f)
      .addComponent<Components::Color>(0.0f, 1.0f, 1.0f, 1.0f); // Make player cyan

    // Create an enemy entity to act as a template
    auto enemy = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"Enemy">>()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Position>(50.0f, 30.0f)
      .addComponent<Components::Health>(100, 100)
      .addComponent<Components::Size>(10.0f, 10.0f)
      .addComponent<Components::Color>(0.0f, 1.0f, 0.0f, 1.0f);

    // --- Cloning Demonstration ---
    // Create a new enemy by cloning the first one
    auto clonedEnemy = Engine::WORLD.cloneEntity(enemy.getId());

    // Check if the clone is valid and then modify it to differentiate it
    if (clonedEnemy.isValid()) {
      // Get the position component of the clone and move it somewhere else
      auto* clonedPos = clonedEnemy.getComponent<Components::Position>();
      if (clonedPos) {
        clonedPos->x = 200.0f;
        clonedPos->y = 150.0f;
      }
    }
    // --- End of Demonstration ---

    // Entity to clear background
    auto background = Engine::WORLD.createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f); // White background

    // Add systems to the world
    Engine::WORLD.addSystem<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.addSystem<MovementSystem>();
		Engine::WORLD.addSystem<PhysicsSystem>();
    Engine::WORLD.addSystem<HealthSystem>();
    Engine::WORLD.addSystem<RectRenderSystem>();
    Engine::WORLD.addSystem<DamageSystem>(); // Add the new system
  }

  void Update() {
    if (Engine::Input::IsKeyPressed(GLFW_KEY_SPACE))
    {
      Engine::WORLD.sendEvent<DamageEvent>(player.getId(), 10);
      // Print the health of all entities with Health component  
      for (const auto& entity : Engine::WORLD.getEntitiesWith<Components::Health>()) {
        auto* health = entity.getComponent<Components::Health>();
        std::cout << "Entity " << entity.getId() << " has health: " << health->value << "\n";
      }
      std::cout << "\n";

      auto bgEntity = Engine::WORLD.getFirstEntityWith<Components::Background>();
      auto* bg_color = bgEntity.getComponent<Components::Color>();

      if (!player.hasComponent<Components::Health>())
        return;

      *bg_color = Components::Color{
        player.getComponent<Components::Health>()->value / 100.0f,
        player.getComponent<Components::Health>()->value / 100.0f,
        player.getComponent<Components::Health>()->value / 100.0f,
        1.0f
      };
    }
  }

  void Draw() {}

  void Free() {
    Engine::WORLD.destroyEntitiesWith<Components::Tag<"GameObjectState">>();

    // Remove systems
    // TODO: make a function to remove all systems at once
    Engine::WORLD.removeSystem<BackgroundSystem>();
    Engine::WORLD.removeSystem<MovementSystem>();
    Engine::WORLD.removeSystem<HealthSystem>();
    Engine::WORLD.removeSystem<RectRenderSystem>();
    Engine::WORLD.removeSystem<DamageSystem>();
  }

  void Unload() {}
};
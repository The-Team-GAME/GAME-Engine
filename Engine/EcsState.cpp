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

void EcsState::Load()
{
}

void EcsState::Init()
{
  // Create player entity
  auto player = Engine::WORLD.createEntity()
    .addComponent<Components::Tag<"Player">>()
    .addComponent<Components::Tag<"GameObjectState">>()
    .addComponent<Components::Position>(10.0f, 20.0f)
    .addComponent<Components::Velocity>(100.0f, 0.0f)
    .addComponent<Components::Health>(100, 100)
    .addComponent<Components::Renderable>();

  // Create an enemy entity to act as a template
  auto enemy = Engine::WORLD.createEntity()
    .addComponent<Components::Tag<"Enemy">>()
    .addComponent<Components::Tag<"GameObjectState">>()
    .addComponent<Components::Position>(50.0f, 30.0f)
    .addComponent<Components::Health>(50, 50)
    .addComponent<Components::Renderable>();

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
    .addComponent<Components::Color>(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

  // Add systems to the world
  Engine::WORLD.addSystem<BackgroundSystem>(); // Background system should be first
  Engine::WORLD.addSystem<MovementSystem>();
  Engine::WORLD.addSystem<HealthSystem>();
}

void EcsState::Update()
{
}

void EcsState::Draw()
{
}

void EcsState::Free()
{
  Engine::WORLD.destroyEntitiesWith<Components::Tag<"GameObjectState">>();

  // Remove systems
  // TODO: make a function to remove all systems at once
  Engine::WORLD.removeSystem<BackgroundSystem>();
  Engine::WORLD.removeSystem<MovementSystem>();
  Engine::WORLD.removeSystem<HealthSystem>();
}

void EcsState::Unload()
{
}
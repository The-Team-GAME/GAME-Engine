#include "EcsState.h"
#include "Graphics.h"
#include <iostream>
#include <glm/glm.hpp>
#include "Core.h"
#include "BasicComponents.h"
#include "MovementSystem.h"
#include "HealthSystem.h"
#include "RenderSystem.h"

void EcsState::Load()
{
}

void EcsState::Init()
{
  // Create player entity
  auto player = Engine::WORLD->createEntity()
                    .addComponent<Components::Tag<"Player">>()
                    .addComponent<Components::Position>(10.0f, 20.0f)
                    .addComponent<Components::Velocity>(1.0f, 0.0f)
                    .addComponent<Components::Health>(100, 100)
                    .addComponent<Components::Renderable>();

  // Create enemy entity
  auto enemy = Engine::WORLD->createEntity()
                   .addComponent<Components::Tag<"Enemy">>()
                   .addComponent<Components::Position>(50.0f, 30.0f)
                   .addComponent<Components::Health>(50, 50)
                   .addComponent<Components::Renderable>();

  // Add systems
  Engine::WORLD->addSystem<MovementSystem>();
  Engine::WORLD->addSystem<HealthSystem>();
  Engine::WORLD->addSystem<RenderSystem>();
}

void EcsState::Update()
{
  Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f);
}

void EcsState::Draw()
{
}

void EcsState::Free()
{
  Engine::WORLD->destroyEntity(Engine::WORLD->getFirstEntityWith<Components::Tag<"Player">>());
  Engine::WORLD->destroyEntity(Engine::WORLD->getFirstEntityWith<Components::Tag<"Enemy">>());
}

void EcsState::Unload()
{
}
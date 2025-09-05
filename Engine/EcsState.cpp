#include "EcsState.h"
#include "Graphics.h"
#include <iostream>
#include <glm/glm.hpp>
#include "Core.h"
#include "Components/BasicComponents.h"
#include "Systems/MovementSystem.h"
#include "Systems/HealthSystem.h"
#include "Systems/RenderSystem.h"

// Define your components
struct Position
{
  float x, y;
};

struct Velocity
{
  float x, y;
};

struct DrawableRect
{
  float width, height;
  glm::vec4 color;
};

void EcsState::Load()
{
}

void EcsState::Init()
{
  Engine::CORE_ECS->registerComponent<Components::Position>();
  Engine::CORE_ECS->registerComponent<Components::Velocity>();
  Engine::CORE_ECS->registerComponent<Components::Health>();
  Engine::CORE_ECS->registerComponent<Components::Renderable>();

  // Create player entity
  auto player = Engine::CORE_ECS->createEntity()
    .addComponent<Components::Position>(10.0f, 20.0f)
    .addComponent<Components::Velocity>(1.0f, 0.0f)
    .addComponent<Components::Health>(100, 100)
    .addComponent<Components::Renderable>();

  // Create enemy entity
  auto enemy = Engine::CORE_ECS->createEntity()
    .addComponent<Components::Position>(50.0f, 30.0f)
    .addComponent<Components::Health>(50, 50)
    .addComponent<Components::Renderable>();

  // Add systems
  Engine::CORE_ECS->addSystem<MovementSystem>();
  Engine::CORE_ECS->addSystem<HealthSystem>();
  Engine::CORE_ECS->addSystem<RenderSystem>();
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
}

void EcsState::Unload()
{
}
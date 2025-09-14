#pragma once

#include "StateManager.h"
#include "Graphics.h"
#include "ObjectFactory.h"
#include "MovementSystem.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "Core.h"

class Rubric2500Particles : public Engine::GameState<Rubric2500Particles>
{
private:
public:
  void Load() {};

  void Init()
  {
    auto background = Engine::WORLD->createEntity()
      .addComponent<Components::Tag<"GameObjectState">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    Engine::WORLD->addSystem<BackgroundSystem>(); // Background system should be first
    Engine::WORLD->addSystem<GameObjectRenderSystem>();
  };

  void Update() {};

  void Draw()
  {
  };

  void Free() {
    Engine::WORLD->destroyEntitiesWith<Components::Tag<"GameObjectState">>();

    // Remove systems
    Engine::WORLD->removeSystem<BackgroundSystem>();
    Engine::WORLD->removeSystem<GameObjectRenderSystem>();
  };

  void Unload() {};
};
#pragma once

#include "StateManager.h"
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
      .addComponent<Components::Tag<"Rubric2500Particles">>()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 2500; ++i) {
      float x = static_cast<float>(rand() % 800); // Random x position within window width
      float y = static_cast<float>(rand() % 600); // Random y position within window height
      float size = static_cast<float>((rand() % 20) + 5); // Random size between 5 and 25
      float rotation = static_cast<float>(rand() % 360); // Random rotation angle
      Components::Color color = Components::Color(
        static_cast<float>(rand() % 256) / 255.0f,
        static_cast<float>(rand() % 256) / 255.0f,
        static_cast<float>(rand() % 256) / 255.0f,
        1.0f
      );
      float rotationSpeed = 30.0f + static_cast<float>(rand() % 90); // Random rotation speed between 30 and 120 degrees per second

      Engine::WORLD->createEntity()
        .addComponent<Components::Tag<"Rubric2500Particles">>()
        .addComponent<Components::Position>(x, y)
        .addComponent<Components::Size>(size, size)
        .addComponent<Components::Color>(color)
        .addComponent<Components::Rotation>(rotation)
        .addComponent<Components::RotationSpeed>(rotationSpeed);
    }


    // Add systems to the world
    Engine::WORLD->addSystem<BackgroundSystem>(); // Background system should be first
    Engine::WORLD->addSystem<RotationSystem>();
    Engine::WORLD->addSystem<RectRenderSystem>();

  };

  void Update() {};

  void Draw()
  {
  };

  void Free() {
    Engine::WORLD->destroyEntitiesWith<Components::Tag<"Rubric2500Particles">>();

    // Remove systems
    Engine::WORLD->removeSystem<BackgroundSystem>();
    Engine::WORLD->removeSystem<RotationSystem>();
    Engine::WORLD->removeSystem<RectRenderSystem>();
  };

  void Unload() {};
};
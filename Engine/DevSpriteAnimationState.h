#pragma once

#include "StateManager.h"
#include "Core.h"
#include "BasicComponents.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include <string>

class DevSpriteAnimationState : public Engine::GameState<DevSpriteAnimationState> {
private:
  ECS::Entity marioEntity;
  ECS::Entity backgroundEntity;

public:
  void Load() {};

  void Init() {
    // Create background entity
    backgroundEntity = Engine::WORLD.createEntity()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.1f, 0.1f, 0.1f, 1.0f);

    // Create animated Mario entity using the AnimatedSprite component
    marioEntity = Engine::WORLD.createEntity()
      .addComponent<Components::Position>(200.0f, 150.0f)
      .addComponent<Components::Size>(96.0f, 96.0f)
      .addComponent<Components::AnimatedSprite>(
        "Assets/images/mario.png",  // sprite sheet path
        96 / 3,                      // frame width (32 pixels)
        32,                          // frame height
        3,                           // total frames
        8.0f,                        // frames per second
        Graphics::MinifyingFilter::Nearest
      );

    marioEntity.addComponent<Components::Rotation>(0.0f);
    marioEntity.addComponent<Components::Scale>(1.0f, 1.0f);
    marioEntity.addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

    // Add systems to render the entities
    Engine::WORLD.addSystem<BackgroundSystem>();
    Engine::WORLD.addSystem<AnimatedSpriteRenderSystem>();
  };

  void Update() {
  };

  void Draw() {
  };

  void Free() {
    // Clean up entities
    if (marioEntity.isValid()) {
      marioEntity.destroy();
    }
    if (backgroundEntity.isValid()) {
      backgroundEntity.destroy();
    }
  };

  void Unload() {

  };
};
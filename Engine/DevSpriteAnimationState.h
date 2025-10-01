#pragma once

#include "StateManager.h"
#include "Core.h"
#include "BasicComponents.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include <string>

class DevSpriteAnimationState : public Engine::GameState<DevSpriteAnimationState> {
private:
  ECS::Entity idleLazyMonkey;
  ECS::Entity walkLazyMonkey;
  ECS::Entity backgroundEntity;

public:
  void Load() {};

  void Init() {
    // Create background entity
    backgroundEntity = Engine::WORLD.createEntity()
      .addComponent<Components::Background>()
      .addComponent<Components::Color>(0.1f, 0.1f, 0.1f, 1.0f);

    // Create animated LazyMonkey entity using the AnimatedSprite component
    idleLazyMonkey = Engine::WORLD.createEntity()
      .addComponent<Components::Position>(200.0f, 150.0f)
      .addComponent<Components::Size>(96.0f, 96.0f).addComponent<Components::Rotation>(0.0f)
      .addComponent<Components::Scale>(1.0f, 1.0f)
      .addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .addComponent<Components::AnimatedSprite>(
        "Assets/images/LazyMonkey_Idle_Spritesheet.png",  // sprite sheet path
        320 / 5,                                          // frame width (Total width / number of frames)
        64,                                               // frame height
        5,                                                // total frames
        8.0f,                                             // frames per second
        Graphics::MinifyingFilter::Nearest
      );


    walkLazyMonkey = Engine::WORLD.createEntity()
      .addComponent<Components::Position>(400.0f, 150.0f)
      .addComponent<Components::Size>(96.0f, 96.0f).addComponent<Components::Rotation>(0.0f)
      .addComponent<Components::Scale>(1.0f, 1.0f)
      .addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .addComponent<Components::AnimatedSprite>(
        "Assets/images/LazyMonkey_Walk_Spritesheet.png",  // sprite sheet path
        384 / 6,                                          // frame width (Total width / number of frames)
        64,                                               // frame height
        6,                                                // total frames
        16.0f,                                            // frames per second
        Graphics::MinifyingFilter::Nearest
      );

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
    if (idleLazyMonkey.isValid()) {
      idleLazyMonkey.destroy();
    }
    if (walkLazyMonkey.isValid()) {
      walkLazyMonkey.destroy();
    }
    if (backgroundEntity.isValid()) {
      backgroundEntity.destroy();
    }
  };

  void Unload() {

  };
};
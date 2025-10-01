#pragma once

#include "StateManager.h"
#include "Core.h"
#include "BasicComponents.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include <string>
#include "SimplePlayerMovementSystem.h"

class DevSpriteAnimationState : public Engine::GameState<DevSpriteAnimationState> {
private:
  ECS::Entity idleLazyMonkey;
  ECS::Entity walkLazyMonkey;
  ECS::Entity backgroundEntity;

  s32 idleSpriteSheetId;
  s32 walkSpriteSheetId;

public:
  void Load() {
    // Load sprite sheets in the Load phase
    idleSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "LazyMonkey_Idle",
      "Assets/images/LazyMonkey_Idle_Spritesheet.png",
      320 / 5,  // frame width
      64,       // frame height
      5,        // total frames
      8.0f,     // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    walkSpriteSheetId = Engine::RESOURCES.load_sprite_sheet(
      "LazyMonkey_Walk",
      "Assets/images/LazyMonkey_Walk_Spritesheet.png",
      384 / 6,  // frame width
      64,       // frame height
      6,        // total frames
      16.0f,    // frames per second
      Graphics::MinifyingFilter::Nearest
    );
  };

  void Init() {
    // Create background entity
    backgroundEntity = Engine::WORLD.create_entity()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.1f, 0.1f, 0.1f, 1.0f);

    // Create animated LazyMonkey entities using sprite sheet IDs
    idleLazyMonkey = Engine::WORLD.create_entity()
      .add_component<Components::Position>(200.0f, 150.0f)
      .add_component<Components::Size>(96.0f, 96.0f)
      .add_component<Components::Rotation>(0.0f)
      .add_component<Components::Scale>(1.0f, 1.0f)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::SimplePlayerControl>(100.0f)
      .add_component<Components::AnimatedSprite>(idleSpriteSheetId);

    walkLazyMonkey = Engine::WORLD.create_entity()
      .add_component<Components::Position>(400.0f, 150.0f)
      .add_component<Components::Size>(96.0f, 96.0f)
      .add_component<Components::Rotation>(0.0f)
      .add_component<Components::Scale>(1.0f, 1.0f)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::AnimatedSprite>(walkSpriteSheetId);

    // Add systems to render the entities
    Engine::WORLD.add_system<BackgroundSystem>();
    Engine::WORLD.add_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.add_system<SimplePlayerMovementSystem>();
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

    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.remove_system<SimplePlayerMovementSystem>();
  };

  void Unload() {
    // Resources are automatically managed by ResourceManager
  };
};
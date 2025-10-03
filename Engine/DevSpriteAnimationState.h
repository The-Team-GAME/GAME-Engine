//-----------------------------------------------------------------------------------
// file:    DevSpriteAnimationState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a development state for testing animated sprite rendering.
//          Demonstrates sprite sheet loading and animation playback with idle and
//          walking animations for a LazyMonkey character.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
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
  ECS::Entity idle_lazy_monkey;
  ECS::Entity walk_lazy_monkey;
  ECS::Entity background_entity;

  s32 idle_sprite_sheet_id;
  s32 walk_sprite_sheet_id;

public:
  void load() {
    // Load sprite sheets in the Load phase
    idle_sprite_sheet_id = Engine::RESOURCES.load_sprite_sheet(
      "LazyMonkey_Idle",
      "Assets/images/LazyMonkey_Idle_Spritesheet.png",
      320 / 5,  // frame width
      64,       // frame height
      5,        // total frames
      8.0f,     // frames per second
      Graphics::MinifyingFilter::Nearest
    );

    walk_sprite_sheet_id = Engine::RESOURCES.load_sprite_sheet(
      "LazyMonkey_Walk",
      "Assets/images/LazyMonkey_Walk_Spritesheet.png",
      384 / 6,  // frame width
      64,       // frame height
      6,        // total frames
      16.0f,    // frames per second
      Graphics::MinifyingFilter::Nearest
    );
  };

  void init() {
    // Create background entity
    background_entity = Engine::WORLD.create_entity()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.1f, 0.1f, 0.1f, 1.0f);

    // Create animated LazyMonkey entities using sprite sheet IDs
    idle_lazy_monkey = Engine::WORLD.create_entity()
      .add_component<Components::Position>(200.0f, 150.0f)
      .add_component<Components::Size>(96.0f, 96.0f)
      .add_component<Components::Rotation>(0.0f)
      .add_component<Components::Scale>(1.0f, 1.0f)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::SimplePlayerControl>(100.0f)
      .add_component<Components::AnimatedSprite>(idle_sprite_sheet_id);

    walk_lazy_monkey = Engine::WORLD.create_entity()
      .add_component<Components::Position>(400.0f, 150.0f)
      .add_component<Components::Size>(96.0f, 96.0f)
      .add_component<Components::Rotation>(0.0f)
      .add_component<Components::Scale>(1.0f, 1.0f)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::AnimatedSprite>(walk_sprite_sheet_id);

    // Add systems to render the entities
    Engine::WORLD.add_system<BackgroundSystem>();
    Engine::WORLD.add_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.add_system<SimplePlayerMovementSystem>();
  };

  void update() {
  };

  void draw() {
  };

  void free() {
    // Clean up entities
    if (idle_lazy_monkey.is_valid()) {
      idle_lazy_monkey.destroy();
    }
    if (walk_lazy_monkey.is_valid()) {
      walk_lazy_monkey.destroy();
    }
    if (background_entity.is_valid()) {
      background_entity.destroy();
    }

    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<AnimatedSpriteRenderSystem>();
    Engine::WORLD.remove_system<SimplePlayerMovementSystem>();
  };

  void unload() {
    // Resources are automatically managed by ResourceManager
  };
};
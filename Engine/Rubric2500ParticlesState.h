//-----------------------------------------------------------------------------------
// file:    Rubric2500ParticlesState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a game state that creates and renders 2500 rotating
//          colored particles to demonstrate ECS performance capabilities as required
//          by the rubric specifications.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "Core.h"

class Rubric2500ParticlesState : public Engine::GameState<Rubric2500ParticlesState>
{
private:
public:
  void Load() {};

  void Init()
  {
    auto background = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"Rubric2500ParticlesState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 2500; ++i) {
      f32 x = static_cast<f32>(rand() % 1600); // Random x position within window width
      f32 y = static_cast<f32>(rand() % 900); // Random y position within window height
      f32 size = static_cast<f32>((rand() % 20) + 5); // Random size between 5 and 25
      f32 rotation = static_cast<f32>(rand() % 360); // Random rotation angle
      Components::Color color = Components::Color(
        static_cast<f32>(rand() % 256) / 255.0f,
        static_cast<f32>(rand() % 256) / 255.0f,
        static_cast<f32>(rand() % 256) / 255.0f,
        1.0f
      );
      f32 rotationSpeed = 30.0f + static_cast<f32>(rand() % 90); // Random rotation speed between 30 and 120 degrees per second

      Engine::WORLD.create_entity()
        .add_component<Components::Tag<"Rubric2500ParticlesState">>()
        .add_component<Components::Position>(x, y)
        .add_component<Components::Size>(size, size)
        .add_component<Components::Color>(color)
        .add_component<Components::Rotation>(rotation)
        .add_component<Components::RotationSpeed>(rotationSpeed);
    }


    // Add systems to the world
    Engine::WORLD.add_system<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.add_system<RotationSystem>();
    Engine::WORLD.add_system<RectRenderSystem>();

  };

  void Update() {};

  void Draw()
  {
  };

  void Free() {
    Engine::WORLD.destroy_entities_with<Components::Tag<"Rubric2500ParticlesState">>();

    // Remove systems
    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<RotationSystem>();
    Engine::WORLD.remove_system<RectRenderSystem>();
  };

  void Unload() {};
};
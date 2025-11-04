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

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Systems/PhysicsSystem.h"

class Rubric2500ParticlesState : public Engine::Scene
{
private:
public:
  void load() {};

  void init()
  {
    for (s32 i = 0; i < 2500; ++i) {
      f32 x = static_cast<f32>(rand() % 1600); // Random x position within window width
      f32 y = static_cast<f32>(rand() % 900); // Random y position within window height
      f32 size = static_cast<f32>((rand() % 20) + 5); // Random size between 5 and 25
      f32 rotation = static_cast<f32>(rand() % 360); // Random rotation angle
      Components::Color color = Components::Color(
        static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f),
        static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f),
        static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f),
        1.0f
      );

      auto temp = Engine::ECS::create_entity()
        .add_component<Components::Tag<"Rubric2500ParticlesState">>()
        .add_component<Components::Transform2D>(x, y, rotation, size, size)
        .add_component<Components::Color>(color)
        .add_component<Components::RigidBody>(Vec2D(x,y), false, false)
        .add_component<Components::Texture>(Engine::Resources::get_rect_texture());

      temp.get_component<Components::RigidBody>()->angular_velocity = 30.0f + static_cast<f32>(rand() % 90); // Random rotation speed between 30 and 120 degrees per second 
    }
  };

  void update()
  {

  }

  void draw()
  {
    Engine::Graphics::clear_screen({ 0.0f,0.0f,0.0f,0.0f }); // Clear to black
  };

  void free() {
    Engine::ECS::destroy_entities_with<Components::Tag<"Rubric2500ParticlesState">>();
  };

  void unload() {};
};
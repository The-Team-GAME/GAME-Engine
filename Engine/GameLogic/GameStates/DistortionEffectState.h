
//-----------------------------------------------------------------------------------
// file:    DistortionEffectState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the DistortionEffectState which demonstrates advanced
//          rendering effects using distortion shaders.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

/* EVERY CODE HERE IS TEMPORARY */

#include "../../Engine/MonkeyEngine.h"

class DistortionEffectState : public Engine::Scene
{
private:

public:
  void load() {


    auto monkey_texture = Engine::Resources::load_texture("images/monkey.png");

    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(500.0f, 300.0f, 0.0f, static_cast<f32>(monkey_texture->width * 30), static_cast<f32>(monkey_texture->height * 30))
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Tag<"DistortionEffectState">>();


    Engine::ECS::create_entity()
      .add_component<Components::Tag<"MainMenuState">>()
      .add_component<Components::Transform2D>(1000.0f, 300.0f, 0.0f, static_cast<f32>(monkey_texture->width * 30), static_cast<f32>(monkey_texture->height * 30))
      .add_component<Components::Texture>(monkey_texture)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Distortion>(0.5f, 2.0f)
      .add_component<Components::Tag<"DistortionEffectState">>();
  };

  void init() {};

  void update() {};

  void draw() {};

  void free() {};

  void unload() {
    Engine::ECS::destroy_entities_with<Components::Tag<"DistortionEffectState">>();
  };
};
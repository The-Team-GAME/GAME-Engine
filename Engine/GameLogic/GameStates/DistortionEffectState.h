
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


#include "pch.h"

class DistortionEffectState : public Engine::Scene
{
private:

public:
  void load() {
    auto texture = Engine::Resources::load_texture("images/BroSpriteSheets/Bro_Idle_Spritesheet.png");

    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(500.0f, 300.0f, 0.0f, static_cast<f32>(texture->width / 24) * 10.0f, static_cast<f32>(texture->height * 10))
      .add_component<Components::Texture>(texture)
      .add_component<Components::TextureAtlas>(static_cast<int>(texture->width / 24), static_cast<int>(texture->height), 1, 24, 24)
      .add_component<Components::Animation>(24, 0.1f)
      .add_component<Components::Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
      .add_component<Components::Tag<"DistortionEffectState">>();


    Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(1000.0f, 300.0f, 0.0f, static_cast<f32>(texture->width / 24) * 10.0f, static_cast<f32>(texture->height * 10))
      .add_component<Components::Texture>(texture)
      .add_component<Components::TextureAtlas>(static_cast<int>(texture->width / 24), static_cast<int>(texture->height), 1, 24, 24)
      .add_component<Components::Animation>(24, 0.1f)
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
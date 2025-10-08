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

#include "../../Engine/MonkeyEngine.h"
#include <string>

class DevSpriteAnimationState : public Engine::GameState<DevSpriteAnimationState> {
private:
  Engine::ECS::Entity idle_lazy_monkey;
  Engine::ECS::Entity walk_lazy_monkey;

  Engine::Graphics::Texture idle_texture;
  Engine::Graphics::Texture walk_texture;

public:
  void load() {

    Engine::Resources::set_resource_path("Assets/");

    // Load sprite sheet textures in the Load phase
    idle_texture = Engine::Resources::load_texture("images/LazyMonkey_Idle_Spritesheet.png");
    walk_texture = Engine::Resources::load_texture("images/LazyMonkey_Walk_Spritesheet.png");
  };

  void init() {
    // Create idle LazyMonkey entity with animation
    idle_lazy_monkey = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(
        glm::vec2(200.0f, 150.0f),  // position
        0.0f,                        // rotation
        glm::vec2(192.0f, 192.0f)    // scale (3x the 64px height)
      )
      .add_component<Components::Texture>(idle_texture)
      .add_component<Components::TextureAtlas>(
        320 / 5,  // subTextureWidth (64px per frame)
        64,       // subTextureHeight
        1,        // rows
        5,        // cols
        5         // totalSubTextures
      )
      .add_component<Components::Animation>(
        5,        // frameCount
        1.0f / 8.0f  // frameDuration (8 fps)
      )
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

    // Set animation start time
    auto* idle_anim = idle_lazy_monkey.get_component<Components::Animation>();
    if (idle_anim) {
      idle_anim->startTime = Engine::Time::get_time();
    }

    // Create walking LazyMonkey entity with animation
    walk_lazy_monkey = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(
        glm::vec2(400.0f, 150.0f),  // position
        0.0f,                        // rotation
        glm::vec2(192.0f, 192.0f)    // scale (3x the 64px height)
      )
      .add_component<Components::Texture>(walk_texture)
      .add_component<Components::TextureAtlas>(
        384 / 6,  // subTextureWidth (64px per frame)
        64,       // subTextureHeight
        1,        // rows
        6,        // cols
        6         // totalSubTextures
      )
      .add_component<Components::Animation>(
        6,        // frameCount
        1.0f / 16.0f  // frameDuration (16 fps)
      )
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

    // Set animation start time
    auto* walk_anim = walk_lazy_monkey.get_component<Components::Animation>();
    if (walk_anim) {
      walk_anim->startTime = Engine::Time::get_time();
    }

    // Add rendering system for animated sprites
    Engine::ECS::add_system<Systems::AtlasSpriteRenderSystem>();
    Engine::ECS::add_system<Systems::AnimationUpdateSystem>();
  };

  void update() {
    // Update is handled by the ECS system
  };

  void draw() {
    // Clear background
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing is handled by the ECS system
  };

  void free() {
    // Clean up entities
    if (idle_lazy_monkey.is_valid()) {
      idle_lazy_monkey.destroy();
    }
    if (walk_lazy_monkey.is_valid()) {
      walk_lazy_monkey.destroy();
    }

    // Remove systems
    Engine::ECS::remove_system<Systems::AtlasSpriteRenderSystem>();
    Engine::ECS::remove_system<Systems::AnimationUpdateSystem>();
  };

  void unload() {
    // Reset texture pointers (actual cleanup handled by resource manager)
    idle_texture.reset();
    walk_texture.reset();

    // Clear unused resources from cache
    Engine::Resources::clear_unused_resources();
  };
};
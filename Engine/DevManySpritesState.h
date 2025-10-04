//-----------------------------------------------------------------------------------
// file:    DevManySpritesState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a development state for stress testing sprite rendering.
//          Creates 2500 rotating banana sprites to test the engine's batch rendering
//          performance and entity management capabilities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h" 
#include "Core.h"
#include "BackgroundSystem.h"
#include "RenderSystem.h"
#include "RotationSystem.h"

/* ================================================= //
// Stress test state for sprite rendering           //
// ================================================= */
class DevManySpritesState : public Engine::GameState<DevManySpritesState>
{
private:
  s32 enemy_prefab = -1;
  s32 banana_prefab = -1;

public:
  void load() {
    // Load object prefabs through ResourceManager  
    enemy_prefab = Engine::RESOURCES.load_object_prefab("enemy", "Assets/objects/enemy.json");
    banana_prefab = Engine::RESOURCES.load_object_prefab("banana", "Assets/objects/banana.json");

    for (s32 i = 0; i < 2500; ++i) { // Create 2500 bananas
      f32 x = static_cast<f32>(rand() % 1600); // Random x position within window width
      f32 y = static_cast<f32>(rand() % 900); // Random y position within window height
      Engine::FACTORY.createObject(banana_prefab)
        .add_component<Components::Tag<"DevManySpritesState">>()
        .add_component<Components::Position>(x, y)
        .add_component<Components::Rotation>(0.0f)
        .add_component<Components::RotationSpeed>(45.0f);
    }

    auto background = ECS::World::create_entity()
      .add_component<Components::Tag<"DevManySpritesState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(0.2f, 0.3f, 0.4f, 1.0f); // Dark blue background

    // Add systems to the world
    ECS::World::add_system<BackgroundSystem>();
    ECS::World::add_system<RotationSystem>();
    ECS::World::add_system<StaticSpriteRenderSystem>();
  }

  void init() {}

  void update() {}

  void draw() {}

  void free() {
    // Unload through ResourceManager (new approach)
    Engine::RESOURCES.unload_object_prefab("enemy");
    Engine::RESOURCES.unload_object_prefab("banana");

    ECS::World::destroy_entities_with<Components::Tag<"DevManySpritesState">>();

    // Remove systems
    ECS::World::remove_system<BackgroundSystem>();
    ECS::World::remove_system<RotationSystem>();
    ECS::World::remove_system<StaticSpriteRenderSystem>();
  }

  void unload() {}
};
//-----------------------------------------------------------------------------------
// file:    GameObjectState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the GameObjectState which demonstrates the ObjectFactory
//          prefab system. Shows how to load prefabs from JSON files and instantiate
//          multiple entities with different component configurations.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

class GameObjectState : public Engine::GameState<GameObjectState>
{
private:
  s32 enemy_prefab = -1;
  s32 banana_prefab = -1;
  s32 lazy_monkey_prefab = -1;

public:
  void load() {}

  void init()
  {
    Engine::Resources::set_resource_path("Assets/");

    // Load prefabs from JSON files
    enemy_prefab = Engine::ObjectFactory::register_object("Assets/objects/enemy.json");
    banana_prefab = Engine::ObjectFactory::register_object("Assets/objects/banana.json");
    lazy_monkey_prefab = Engine::ObjectFactory::register_object("Assets/objects/lazy_monkey.json");

    Engine::ECS::add_system<Systems::StaticSpriteRenderSystem>();

    // Create enemies from prefab
    auto enemy1 = Engine::ObjectFactory::create_object(enemy_prefab)
      .add_component<Components::Tag<"GameObjectState">>();

    // Modify enemy1's transform
    if (auto* transform = enemy1.get_component<Components::Transform2D>()) {
      transform->position = Math::Vec2D(200.0f, 300.0f);
    }

    auto enemy2 = Engine::ObjectFactory::create_object(enemy_prefab)
      .add_component<Components::Tag<"GameObjectState">>();

    // Modify enemy2's transform - different position and scale
    if (auto* transform = enemy2.get_component<Components::Transform2D>()) {
      transform->position = Math::Vec2D(600.0f, 300.0f);
      transform->scale = Math::Vec2D(2.0f, 2.0f); // Make it 2x bigger
    }

    // Create bananas
    auto banana1 = Engine::ObjectFactory::create_object(banana_prefab)
      .add_component<Components::Tag<"GameObjectState">>();

    if (auto* transform = banana1.get_component<Components::Transform2D>()) {
      transform->position = Math::Vec2D(400.0f, 200.0f);
      transform->rotation = 45.0f; // Rotate 45 degrees
    }

    auto banana2 = Engine::ObjectFactory::create_object(banana_prefab)
      .add_component<Components::Tag<"GameObjectState">>();

    if (auto* transform = banana2.get_component<Components::Transform2D>()) {
      transform->position = Math::Vec2D(450.0f, 250.0f);
      transform->scale = Math::Vec2D(3.0f, 3.0f); // Make it 3x bigger
    }

    // Create animated lazy monkey
    auto lazy_monkey = Engine::ObjectFactory::create_object(lazy_monkey_prefab)
      .add_component<Components::Tag<"GameObjectState">>();

    if (auto* transform = lazy_monkey.get_component<Components::Transform2D>()) {
      transform->position = Math::Vec2D(800.0f, 450.0f);
      transform->scale = Math::Vec2D(3.0f, 3.0f); // Scale it up to see it better
    }

    // Tint the monkey red
    if (auto* color = lazy_monkey.get_component<Components::Color>()) {
      color->color = Math::Vec4D(1.0f, 0.5f, 0.5f, 1.0f); // Red tint
    }
  }

  void update() {}

  void draw()
  {
    // Clear screen with a color
    Engine::Graphics::clear_screen(Math::Vec4D(0.2f, 0.3f, 0.5f, 1.0f)); // Dark blue background
  }

  void free()
  {
    // Unregister prefabs
    Engine::ObjectFactory::unregister_object(enemy_prefab);
    Engine::ObjectFactory::unregister_object(banana_prefab);
    Engine::ObjectFactory::unregister_object(lazy_monkey_prefab);

    Engine::ECS::remove_system<Systems::StaticSpriteRenderSystem>();

    // Destroy all entities with the GameObjectState tag
    Engine::ECS::destroy_entities_with<Components::Tag<"GameObjectState">>();
  }

  void unload() {}
};
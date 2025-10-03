//-----------------------------------------------------------------------------------    
// file:    DevECSMessageState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a development state for testing ECS event messaging
//          and entity cloning. Demonstrates damage events, health system integration,
//          and dynamic background color changes based on player health.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h"
#include "PhysicsTestState.h"
#include "Graphics.h"
#include <iostream>
#include <glm/glm.hpp>
#include "Core.h"
#include "BasicComponents.h"
#include "MovementSystem.h"
#include "HealthSystem.h"
#include "RenderSystem.h"
#include "BackgroundSystem.h"
#include "InputManager.h"
#include "BasicEvents.h"
#include "DamageSystem.h"

class DevECSMessageState : public Engine::GameState<DevECSMessageState>
{
private:
  s32 enemy_prefab = -1;
  ECS::Entity player;

public:
  void load() {
    enemy_prefab = Engine::FACTORY.registerObject("Assets/objects/enemy.json");
  }

  void init() {
    // Create player entity
    player = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"Player">>()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(10.0f, 20.0f)
      .add_component<Components::Collider>(10.0f, 10.0f)
      .add_component<Components::RigidBody>(1.0f, false) // mass=1.0, not static
      .add_component<Components::Health>(100, 100)
      .add_component<Components::Size>(10.0f, 10.0f)
      .add_component<Components::Color>(0.0f, 1.0f, 1.0f, 1.0f); // Make player cyan

    player.get_component<Components::RigidBody>()->velocity.X() = 100.0f; // Give the player an initial horizontal velocity

    // Create an enemy entity to act as a template
    auto enemy = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"Enemy">>()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Position>(50.0f, 30.0f)
      .add_component<Components::Health>(100, 100)
      .add_component<Components::Size>(10.0f, 10.0f)
      .add_component<Components::Color>(0.0f, 1.0f, 0.0f, 1.0f);

    // --- Cloning Demonstration ---
    // Create a new enemy by cloning the first one
    auto clonedEnemy = Engine::WORLD.clone_entity(enemy.get_id());

    // Check if the clone is valid and then modify it to differentiate it
    if (clonedEnemy.is_valid()) {
      // Get the position component of the clone and move it somewhere else
      auto* clonedPos = clonedEnemy.get_component<Components::Position>();
      if (clonedPos) {
        clonedPos->X() = 200.0f;
        clonedPos->Y() = 150.0f;
      }
    }
    // --- End of Demonstration ---

    // Entity to clear background
    auto background = Engine::WORLD.create_entity()
      .add_component<Components::Tag<"GameObjectState">>()
      .add_component<Components::Background>()
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f); // White background

    // Add systems to the world
    Engine::WORLD.add_system<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.add_system<GravitySystem>();
    Engine::WORLD.add_system<PlayerControllerSystem>();
    Engine::WORLD.add_system<RectRenderSystem>();
    Engine::WORLD.add_system<MovementSystem>();
    Engine::WORLD.add_system<PhysicsSystem>();
    Engine::WORLD.add_system<CollisionSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();
    Engine::WORLD.add_system<HealthSystem>();


    Engine::WORLD.add_system<DamageSystem>(); // Add the new system
  }

  void update() {
    if (Engine::Input::is_key_pressed(GLFW_KEY_SPACE))
    {
      Engine::WORLD.send_event<DamageEvent>(player.get_id(), 10);
      // Print the health of all entities with Health component  
      for (const auto& entity : Engine::WORLD.get_entities_with<Components::Health>()) {
        auto* health = entity.get_component<Components::Health>();
        std::cout << "Entity " << entity.get_id() << " has health: " << health->value << "\n";
      }
      std::cout << "\n";

      auto bgEntity = Engine::WORLD.get_first_entity_with<Components::Background>();
      auto* bg_color = bgEntity.get_component<Components::Color>();

      if (!player.has_component<Components::Health>())
        return;

      *bg_color = Components::Color{
        player.get_component<Components::Health>()->value / 100.0f,
        player.get_component<Components::Health>()->value / 100.0f,
        player.get_component<Components::Health>()->value / 100.0f,
        1.0f
      };
    }
  }

  void draw() {}

  void free() {
    Engine::WORLD.destroy_entities_with<Components::Tag<"GameObjectState">>();

    // Remove systems
    // TODO: make a function to remove all systems at once
    Engine::WORLD.remove_system<BackgroundSystem>(); // Background system should be first
    Engine::WORLD.remove_system<GravitySystem>();
    Engine::WORLD.remove_system<PlayerControllerSystem>();
    Engine::WORLD.remove_system<MovementSystem>();
    Engine::WORLD.remove_system<PhysicsSystem>();
    Engine::WORLD.remove_system<CollisionSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
    Engine::WORLD.remove_system<HealthSystem>();
    Engine::WORLD.remove_system<RectRenderSystem>();

    Engine::WORLD.remove_system<DamageSystem>(); // Add the new system
  }

  void unload() {}
};
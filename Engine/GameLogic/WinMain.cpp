//-----------------------------------------------------------------------------------
// file:    WinMain.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file contains the main entry point for the game engine. It sets up
//          memory leak detection, initializes the core engine, and handles top-level
//          exception catching for robust error reporting.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#include "../Engine/MonkeyEngine.h"
#include "GameStates/EmptyState.h"
#include "GameStates/MainMenuState.h"
#include "GameStates/ShowcaseState.h"
#include "GameStates/RectPainterState.h"
#include "GameStates/Rubric2500ParticlesState.h"
#include "GameStates/StressTestState.h"
#include "GameStates/BasicState.h"
#include "GameStates/PrefabsState.h"
#include "GameStates/PhysicsTestState.h"
#include "GameStates/TimelineState.h"
#include "GameStates/DistortionEffectState.h"
#include "GameStates/LuaState.h"
#include "GameStates/BehaviourState.h"
#include "GameStates/LevelTestState.h"
#include "Systems/HealthSystem.h"
#include "Systems/EnemyHealthSystem.h"
#include "Systems/HeartrateSystem.h"
#include "Systems/DebugHitboxRenderSystem.h"

/*=================================================================================
Function Brief:
Main entry point for the application. Enables memory leak detection, initializes
and runs the game engine, and catches any unhandled exceptions for error logging.

Returns:
0 on successful execution, 1 if an exception occurred.
==================================================================================*/
int main()
{
  // Enable memory leak detection
  s32 tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(tmpDbgFlag);
  _CrtSetBreakAlloc(-1);


  // Register game layers
  Engine::ECS::register_layer("background"); // Static background layer
  //Engine::ECS::register_layer("tilemap_background_far");  // Tilemap background far layer (e.g., distant cityscape, outside window) // TODO: Re-enable after M3
  Engine::ECS::register_layer("tilemap_background_mid");  // Tilemap background mid layer (e.g., MRT carriage, trees, hills, pillars)
  //Engine::ECS::register_layer("tilemap_background_near"); // Tilemap background near layer (e.g., bushes, fences) // TODO: Re-enable after M3
  // Engine::ECS::register_layer("tilemap_foreground_far"); // Tilemap foreground far layer (don't need it for now)
  Engine::ECS::register_layer("tilemap_foreground_mid"); // Tilemap foreground mid layer (e.g., things that needs to collide with player, ground, platforms)
  //Engine::ECS::register_layer("tilemap_foreground_near"); // Tilemap foreground near layer (don't need it for now)
  Engine::ECS::register_layer("hazard"); // Hazard layer (e.g., spikes, lava) 
  Engine::ECS::register_layer("damage_enemy"); // Damage enemy layer (e.g., hitboxes for player attacks)
  Engine::ECS::register_layer("damage_player"); // Damage player layer (e.g., hitboxes for enemy attacks, hazards)
  Engine::ECS::register_layer("enemy"); // Enemy characters layer
  Engine::ECS::register_layer("player_trail"); // Specific layer for player trail effect
  Engine::ECS::register_layer("player"); // Player character layer
  Engine::ECS::register_layer("effect"); // Visual effects like particles
  Engine::ECS::register_layer("ui"); // User interface elements
  Engine::ECS::register_layer("debug"); // Debug overlays and information

  // Start the engine
  Engine::Core::init();

  // Example: Register a collision callback between gameplay and ui layers
  Engine::ECS::LayerRegistry::register_collision_callback("gameplay", "ui",
    [](Engine::ECS::Entity entityA, Engine::ECS::Entity entityB)
    {
      // Example callback: Print entity IDs on collision
      // In practice, you would handle damage, effects, etc. here
      std::cout << "Collision detected between Entity "
        << entityA.get_id() << " and Entity "
        << entityB.get_id() << std::endl;
    });

  // Register collision callback for player attacks hitting enemies
  Engine::ECS::LayerRegistry::register_collision_callback("damage_enemy", "enemy",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity enemy_entity)
    {
      // When player attack hitbox collides with enemy
      auto* attack = attack_entity.get_component<Components::Attack>();
      auto* enemy_health = enemy_entity.get_component<Components::EnemyHealth>();
      auto* state_machine = enemy_entity.get_component<Components::StateMachine>();
      auto* transform = enemy_entity.get_component<Components::Transform2D>();

      if (!attack) {
        std::cout << "Error: Attack component missing from attack hitbox!" << std::endl;
        return;
      }

      if (enemy_health && state_machine) {
        // Check if this enemy has already been hit by this attack
        if (std::find(attack->hit_entities.begin(), attack->hit_entities.end(), enemy_entity) != attack->hit_entities.end()) {
          // Already hit, skip
          return;
        }

        // Mark entity as hit by this attack
        attack->hit_entities.push_back(enemy_entity);

        // Only take damage if not invulnerable
        int damage = attack->damage;
        enemy_health->take_damage(damage);
        std::cout << "Enemy took " << damage << " damage! HP: "
          << enemy_health->current_hp << "/" << enemy_health->max_hp << std::endl;

        // Trigger hit or death state
        if (enemy_health->current_hp <= 0) {
          if (state_machine->get_current_state() != "Death") {
            state_machine->change_state("Death");
          }
        }
        else if (state_machine->get_current_state() != "Hit") {
          state_machine->change_state("Hit");
        }

        // Attach enemy to hitbox (sticky knockback)
        auto* rigid = enemy_entity.get_component<Components::RigidBody>();

        if (rigid) {
          // Make enemy child of hitbox (offset will be auto-calculated by HierarchySystem)
          enemy_entity.add_component<Components::Parent>(attack_entity);

          // Store knockback to apply when detaching (from Player struct via Attack component)
          enemy_entity.add_component<Components::PendingKnockback>(attack->knockback);

          // Move slightly towards the center of the hitbox (20%)
          auto* attack_transform = attack_entity.get_component<Components::Transform2D>();
          if (attack_transform) {
            Math::Vec2D dir = attack_transform->position - transform->position;
            transform->position += dir * 0.2f;
          }

          // Offset slightly upwards each hit
          transform->position.y += -10.0f;

          // Zero out velocity for clean stick effect
          rigid->velocity.zero();
        }
      }
      else {
        std::cout << "Error: EnemyHealth or StateMachine component missing!" << std::endl;
      }
    });

  // Register collision callback for enemy attacks hitting player
  Engine::ECS::LayerRegistry::register_collision_callback("damage_player", "player",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity player_entity)
    {
      std::cout << "Player took damage!" << std::endl;

      // When enemy attack hitbox collides with player
      auto* attack = attack_entity.get_component<Components::Attack>();
      auto* heartrate = player_entity.get_component<Components::Heartrate>();
      auto* rigid = player_entity.get_component<Components::RigidBody>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();
      auto* attack_transform = attack_entity.get_component<Components::Transform2D>();

      if (!attack) {
        std::cout << "Error: Attack component missing from enemy attack hitbox!" << std::endl;
        return;
      }

      if (heartrate && rigid) {
        // Check if this player has already been hit by this attack
        if (std::find(attack->hit_entities.begin(), attack->hit_entities.end(), player_entity) != attack->hit_entities.end()) {
          // Already hit, skip
          return;
        }

        // Mark entity as hit by this attack
        attack->hit_entities.push_back(player_entity);

        // Increase heartrate instead of decreasing HP
        int damage = attack->damage;
        heartrate->increase(static_cast<float>(damage));
        std::cout << "Player took " << damage << " damage! Heartrate: " << heartrate->heartrate << std::endl;

        // Apply knockback
        if (player_transform && attack_transform) {
          // Determine knockback direction (away from attack)
          float dx = player_transform->position.x - attack_transform->position.x;
          float knockback_direction = (dx > 0 ? 1.0f : -1.0f);
          
          // Apply knockback velocity
          rigid->velocity.x = attack->knockback.x * knockback_direction;
          rigid->velocity.y = attack->knockback.y; // Usually negative for upward knockback
          
          std::cout << "Player knocked back! Velocity: (" << rigid->velocity.x << ", " << rigid->velocity.y << ")" << std::endl;
        }
      }
      else {
        std::cout << "Error: Heartrate or RigidBody component missing from player!" << std::endl;
      }
    });

  // Register game scenes at startup
  Engine::SceneManager::register_scene<EmptyState>("EmptyState");
  Engine::SceneManager::register_scene<MainMenuState>("MainMenuState");
  // Engine::SceneManager::register_scene<ShowcaseState>("ShowcaseState");
  Engine::SceneManager::register_scene<RectPainterState>("RectPainterState");
  Engine::SceneManager::register_scene<Rubric2500ParticlesState>("Rubric2500ParticlesState");
  Engine::SceneManager::register_scene<StressTestState>("StressTestState");
  Engine::SceneManager::register_scene<BasicState>("BasicState");
  Engine::SceneManager::register_scene<PrefabsState>("PrefabsState");
  Engine::SceneManager::register_scene<PhysicsTestState>("PhysicsTestState");
  Engine::SceneManager::register_scene<TimelineState>("TimelineState");
  Engine::SceneManager::register_scene<DistortionEffectState>("DistortionEffectState");
  Engine::SceneManager::register_scene<LuaState>("LuaState");
  Engine::SceneManager::register_scene<LevelTestState>("LevelTestState");
  Engine::SceneManager::register_scene<BehaviourState>("BehaviourState");

  // Register game systems at startup
  Engine::ECS::add_system<Engine::Systems::HealthSystem>();
  Engine::ECS::add_system<Engine::Systems::EnemyHealthSystem>();
  Engine::ECS::add_system<Engine::Systems::HeartrateSystem>();
  Engine::ECS::add_system<Engine::Systems::DebugHitboxRenderSystem>();

  // Run the main menu state
  Engine::Core::run("MainMenuState");

  // Shutdown the engine
  Engine::Core::exit();

  return 0;
}

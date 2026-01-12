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
#include "GameStates/Rubric2500ParticlesState.h"
#include "GameStates/StressTestState.h"
#include "GameStates/BasicState.h"
#include "GameStates/PhysicsTestState.h"
#include "GameStates/TimelineState.h"
#include "GameStates/DistortionEffectState.h"
#include "GameStates/LuaState.h"
#include "GameStates/BehaviourState.h"
#include "GameStates/LevelTestState.h"
#include "GameStates/GameplayState.h"
#include "GameStates/GameMonkeyTestState.h"
#include "GameStates/PauseState.h"
#include "GameStates/OptionsState.h"
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
/*=================================================================================
Function Brief:
Common game initialization and loop logic.
==================================================================================*/
void RunGame()
{
#if defined(_DEBUG)
  // Enable memory leak detection
  s32 tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(tmpDbgFlag);
  _CrtSetBreakAlloc(-1);
#endif

  // Register game layers
  Engine::ECS::register_layer("background"); // Static background layer
  //Engine::ECS::register_layer("tilemap_background_far");  // Tilemap background far layer (e.g., distant cityscape, outside window) // TODO: Re-enable after M3
  Engine::ECS::register_layer("tilemap_background_mid");  // Tilemap background mid layer (e.g., MRT carriage, trees, hills, pillars)
  Engine::ECS::register_layer("tilemap_background_near"); // Tilemap background near layer (e.g., bushes, fences) // TODO: Re-enable after M3
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
  Engine::ECS::register_layer("ui_background"); // User interface elements
  Engine::ECS::register_layer("ui"); // User interface elements
  Engine::ECS::register_layer("ui_foreground"); // User interface elements
#ifndef GAME_BUILD
  Engine::ECS::register_layer("debug"); // Debug overlays and information
#endif



  // Start the engine
  Engine::Core::init();

  // Example: Register a collision callback between gameplay and ui layers
  Engine::ECS::LayerRegistry::register_collision_callback("gameplay", "ui",
    [](Engine::ECS::Entity entityA, Engine::ECS::Entity entityB)
    {
      // Example callback: Print entity IDs on collision
      // In practice, you would handle damage, effects, etc. here
      Log::info("Collision detected between Entity {} and Entity {}", entityA.get_id(), entityB.get_id());
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
        Log::error("Error: Attack component missing from attack hitbox, on entity {}", attack_entity.get_id());
        return;
      }

      if (enemy_health && state_machine) {
        // Check if we should apply damage
        if (!attack->should_apply_damage(enemy_entity)) {
          return;
        }

        // Only take damage if not invulnerable
        int damage = attack->damage;
        enemy_health->take_damage(damage);
        Log::info("Enemy took {} damage! HP: {}/{}", damage, enemy_health->current_hp, enemy_health->max_hp);

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

        // Spawn Impact VFX
        if (attack->impact_vfx_texture.texture_handle) {
          auto vfx = Engine::ECS::create_entity()
            .add_component<Components::Transform2D>(
              transform->position.x,
              transform->position.y,
              0.0f,
              128.0f, // 2x scale
              128.0f
            )
            .add_component<Components::Parent>(enemy_entity)
            .add_component<Components::Texture>(attack->impact_vfx_texture)
            .add_component<Components::TextureAtlas>(64, 64, 1, 4, 4) // 4 frames, 64x64 each
            .add_component<Components::Animation>(0, 3, 0.05f, false)
            .add_component<Components::Lifetime>(0.2f) // 4 * 0.05 = 0.2s
            .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
            .add_to_layer("effect");

          // Randomize rotation for variety?
          // vfx.get_component<Components::Transform2D>()->rotation = ...
        }
      }
      else {
        Log::error("Error: EnemyHealth or StateMachine component missing!");
      }
    });

  // Register collision callback for enemy attacks hitting player
  Engine::ECS::LayerRegistry::register_collision_callback("damage_player", "player",
    [](Engine::ECS::Entity attack_entity, Engine::ECS::Entity player_entity)
    {
      Log::info("Player took damage!");

      // When enemy attack hitbox collides with player
      auto* attack = attack_entity.get_component<Components::Attack>();
      auto* heartrate = player_entity.get_component<Components::Heartrate>();
      auto* rigid = player_entity.get_component<Components::RigidBody>();
      // auto* player_transform = player_entity.get_component<Components::Transform2D>();
      // auto* attack_transform = attack_entity.get_component<Components::Transform2D>();

      if (!attack) {
        Log::error("Error: Attack component missing from enemy attack hitbox, on entity {}", attack_entity.get_id());
        return;
      }

      if (heartrate && rigid) {
        // Check if we should apply damage
        if (!attack->should_apply_damage(player_entity)) {
          return;
        }

        // Increase heartrate instead of decreasing HP
        int damage = attack->damage;
        heartrate->increase(static_cast<float>(damage));
        Log::info("Player took {} damage! Heartrate: {}", damage, heartrate->heartrate);

        // Apply knockback (direction is already baked into attack->knockback from the attacker)
        rigid->velocity.x = attack->knockback.x;
        rigid->velocity.y = attack->knockback.y; // Usually negative for upward knockback

        Log::info("Player knocked back! Velocity: ({}, {})", rigid->velocity.x, rigid->velocity.y);
      }
      else {
        Log::error("Error: Heartrate or RigidBody component missing from player!");
      }
    });


  Engine::FunctionRegistry<void()>::register_function("pause_menu_resume", []() {
    Log::info("Resuming game from pause menu");
    Engine::SceneManager::pop_scene();
    });
  Engine::FunctionRegistry<void()>::register_function("pause_menu_restart", []() {
    Log::info("Restarting level from pause menu");
    Engine::SceneManager::load_scene("GameplayState");
    });
  Engine::FunctionRegistry<void()>::register_function("pause_menu_options", []() {
    Log::info("Opening options from pause menu");
    // Engine::SceneManager::push_scene("OptionsState");
    });
  Engine::FunctionRegistry<void()>::register_function("pause_menu_exit", []() {
    Log::info("Exiting to main menu from pause menu");
    Engine::SceneManager::load_scene("MainMenuState");
    });
  Engine::FunctionRegistry<void(Engine::ECS::Entity)>::register_function("pause_menu_button_hover", [](Engine::ECS::Entity entity) {
    auto* color = entity.get_component<Components::Color>();
    if (color) {
      color->a = 1.0f;
    }
    });

  Engine::FunctionRegistry<void(Engine::ECS::Entity)>::register_function("pause_menu_button_exit_hover", [](Engine::ECS::Entity entity) {
    auto* color = entity.get_component<Components::Color>();
    if (color) {
      color->a = 0.0f;
    }
    });

  // Register game scenes at startup
  Engine::SceneManager::register_scene<EmptyState>("EmptyState");
  Engine::SceneManager::register_scene<MainMenuState>("MainMenuState");
  Engine::SceneManager::register_scene<Rubric2500ParticlesState>("Rubric2500ParticlesState");
  Engine::SceneManager::register_scene<StressTestState>("StressTestState");
  Engine::SceneManager::register_scene<BasicState>("BasicState");
  Engine::SceneManager::register_scene<PhysicsTestState>("PhysicsTestState");
  Engine::SceneManager::register_scene<TimelineState>("TimelineState");
  Engine::SceneManager::register_scene<DistortionEffectState>("DistortionEffectState");
  Engine::SceneManager::register_scene<LuaState>("LuaState");
  Engine::SceneManager::register_scene<LevelTestState>("LevelTestState");
  Engine::SceneManager::register_scene<BehaviourState>("BehaviourState");
  Engine::SceneManager::register_scene<GameMonkeyTestState>("GameMonkeyTestState");
  Engine::SceneManager::register_scene<PauseState>("PauseState");
  Engine::SceneManager::register_scene<OptionsState>("OptionsState");

  // Register game systems at startup
  Engine::ECS::add_system<Engine::Systems::HealthSystem>();
  Engine::ECS::add_system<Engine::Systems::EnemyHealthSystem>();
  Engine::ECS::add_system<Engine::Systems::HeartrateSystem>();
  Engine::ECS::add_system<Engine::Systems::DebugHitboxRenderSystem>();
  Engine::ECS::add_system<Engine::Systems::EnemyHealthBarRectSystem>();
  Engine::ECS::add_system<Engine::Systems::HeartHUDSpriteSystem>();

  // Run the main menu state
  Engine::Core::run("MainMenuState");

  // Shutdown the engine
  Engine::Core::exit();
}

#ifdef GAME_BUILD

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

/*=================================================================================
Function Brief:
WinMain entry point for the application (Game Build).
==================================================================================*/
int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInst, [[maybe_unused]] HINSTANCE hInstPrev, [[maybe_unused]] PSTR cmdline, [[maybe_unused]] int cmdshow)
{
  RunGame();
  return 0;
}

#else

/*=================================================================================
Function Brief:
Main entry point for the application (Console Build).
==================================================================================*/
int main()
{
  RunGame();
  return 0;
}

#endif

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
#include "Systems/HealthSystem.h"
#include "GameStates/LevelTestState.h"

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


  // Start the engine
  Engine::Core::init();

  // Register game layers at startup
  Engine::ECS::register_layer("background"); // Static background layer
  Engine::ECS::register_layer("tilemap_background_far");  // Tilemap background far layer (e.g., distant cityscape, outside window)
  Engine::ECS::register_layer("tilemap_background_mid");  // Tilemap background mid layer (e.g., MRT carriage, trees, hills, pillars)
  Engine::ECS::register_layer("tilemap_background_near"); // Tilemap background near layer (e.g., bushes, fences)
  // Engine::ECS::register_layer("tilemap_foreground_far"); // Tilemap foreground far layer (don't need it for now)
  // Engine::ECS::register_layer("tilemap_foreground_mid"); // Tilemap foreground mid layer (don't need it for now)
  Engine::ECS::register_layer("tilemap_foreground_near"); // Tilemap foreground near layer (e.g., things that needs to collide with player, ground, platforms)
  Engine::ECS::register_layer("hazard"); // Hazard layer (e.g., spikes, lava) 
  Engine::ECS::register_layer("player"); // Player character layer
  Engine::ECS::register_layer("enemy"); // Enemy characters layer
  Engine::ECS::register_layer("player_attack"); // Player attack layer (contains the hitboxes for player attacks)
  Engine::ECS::register_layer("enemy_attack"); // Enemy attack layer (contains the hitboxes for enemy attacks)
  Engine::ECS::register_layer("effect"); // Visual effects like particles
  Engine::ECS::register_layer("ui"); // User interface elements
  Engine::ECS::register_layer("debug"); // Debug overlays and information

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

  // Register game scenes at startup
  Engine::SceneManager::register_scene<EmptyState>("EmptyState");
  Engine::SceneManager::register_scene<MainMenuState>("MainMenuState");
  Engine::SceneManager::register_scene<ShowcaseState>("ShowcaseState");
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

  // Run the main menu state
  Engine::Core::run("MainMenuState");

  // Shutdown the engine
  Engine::Core::exit();

  return 0;
}

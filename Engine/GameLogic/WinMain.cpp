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
#include "GameStates/Documentation/PrefabsState.h"
#include "GameStates/Dev/PhysicsTestState.h"
#include "GameStates/TimelineState.h"
#include "GameStates/DistortionEffectState.h"
#include "GameStates/LuaState.h"

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
  Engine::ECS::register_layer("tilemap_background"); // Tilemap background layer
  Engine::ECS::register_layer("tilemap_foreground"); // Tilemap foreground layer
  Engine::ECS::register_layer("gameplay"); // Entities, items, characters
  Engine::ECS::register_layer("ui"); // User interface elements
  Engine::ECS::register_layer("effects"); // Visual effects like particles
  Engine::ECS::register_layer("debug"); // Debug overlays and information

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

  Engine::Core::run("MainMenuState");

  Engine::Core::exit();


  return 0;
}

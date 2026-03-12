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


#include "pch.h"

// Forward-declare the registration function defined in GameLogicEntry.cpp
void RegisterGameLogic();

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
#ifndef GAME_BUILD
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
  Engine::ECS::register_layer("projectiles"); // Projectiles layer (e.g., bullets, arrows)
  Engine::ECS::register_layer("spray"); // Special layer for Spray can monkey spray clouds
  Engine::ECS::register_layer("enemy"); // Enemy characters layer
  Engine::ECS::register_layer("player_trail"); // Specific layer for player trail effect
  Engine::ECS::register_layer("player"); // Player character layer
  Engine::ECS::register_layer("effect"); // Visual effects like particles
  Engine::ECS::register_layer("main_menu_0"); // Main menu: sky background
  Engine::ECS::register_layer("main_menu_1"); // Main menu: buildings
  Engine::ECS::register_layer("main_menu_2"); // Main menu: carriage, title
  Engine::ECS::register_layer("main_menu_3"); // Main menu: characters (Bro, Monkey)
  Engine::ECS::register_layer("main_menu_4"); // Main menu: front overlay
  Engine::ECS::register_layer("main_menu_5"); // Main menu: buttons, interactive UI
  Engine::ECS::register_layer("main_menu_6"); // Main menu: text any key
  Engine::ECS::register_layer("upgrade_ui_bg"); // Upgrade picker: dimmer overlay
  Engine::ECS::register_layer("upgrade_ui_fg"); // Upgrade picker: cards, panels, particles
#ifndef GAME_BUILD
  Engine::ECS::register_layer("debug"); // Debug overlays and information
#endif



  // Start the engine
  Engine::Core::init();

  // Register all game logic (collision callbacks, scenes, systems, function callbacks)
  RegisterGameLogic();

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

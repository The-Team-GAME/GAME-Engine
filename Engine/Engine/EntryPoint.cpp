//-----------------------------------------------------------------------------------
// file:    EntryPoint.cpp
// project: Monkey Stole My GPA
//
// brief:   Main entry point for the Engine executable. Initializes the core
//          engine, loads GameLogic.dll via the HotReloadManager, and runs the
//          main game loop. Hot-reload checks happen each frame in Debug builds.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "MonkeyEngine.h"
#include "HotReloadManager.h"
#include "ECS/ECS.h"
#include "EventManager/EventManager.h"
#include "FunctionRegistry.h"
#include "Components/StateMachineBlueprintRegistry.h"

namespace {
  constexpr const char* PRELOADED_GAMEPLAY_LEVEL = "levels/LevelOneTestingExtra.json";
}

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

  // ── Register game layers ──────────────────────────────────────────────
  // Layers define rendering order and collision groups.
  // These must be registered before Core::init() and stay fixed across
  // hot-reloads (changing layers would invalidate entity signatures).

  Engine::ECS::register_layer("background");
  Engine::ECS::register_layer("tilemap_background_mid");
  Engine::ECS::register_layer("tilemap_background_near");
  Engine::ECS::register_layer("tilemap_foreground_mid");
  Engine::ECS::register_layer("hazard");
  Engine::ECS::register_layer("damage_enemy");
  Engine::ECS::register_layer("damage_player");
  Engine::ECS::register_layer("projectiles");
  Engine::ECS::register_layer("spray");
  Engine::ECS::register_layer("enemy");
  Engine::ECS::register_layer("player_trail");
  Engine::ECS::register_layer("player");
  Engine::ECS::register_layer("effect");
  Engine::ECS::register_layer("main_menu_0");
  Engine::ECS::register_layer("main_menu_1");
  Engine::ECS::register_layer("main_menu_2");
  Engine::ECS::register_layer("main_menu_3");
  Engine::ECS::register_layer("main_menu_4");
  Engine::ECS::register_layer("main_menu_5");
  Engine::ECS::register_layer("main_menu_6");
  Engine::ECS::register_layer("upgrade_ui_bg"); // Upgrade picker: dimmer overlay
  Engine::ECS::register_layer("upgrade_ui_fg"); // Upgrade picker: cards, panels, particles
#ifndef GAME_BUILD
  Engine::ECS::register_layer("debug");
#endif

  // ── Initialize engine ───────────────────────────────────────────────
  Engine::Core::init();

  // ── Load GameLogic DLL ──────────────────────────────────────────────
  // The DLL exports RegisterGameLogic() which registers all game-specific
  // callbacks, scenes, systems, and collision handlers.
  //
  // If the DLL already exists on disk, load it immediately and kick off
  // a background rebuild in case it is outdated. The source watcher /
  // check_and_reload() will hot-swap once the build finishes.
  // If the DLL does not exist, build it first (synchronously) before loading.
#ifndef GAME_BUILD
  if (!Engine::HotReload::dll_exists())
  {
    Log::info("[EntryPoint] GameLogic.dll not found — building...");
    Engine::HotReload::build_game_logic();
  }
#endif

  if (!Engine::HotReload::load_game_logic())
  {
    Log::error("[EntryPoint] Failed to load GameLogic.dll — aborting");
    Engine::Core::exit();
    return;
  }

  // ── Load prefabs / assets ────────────────────────────────────────────
  // Must happen AFTER loading the DLL so that GameLogic components
  // (Player, LazyMonkey, EnemyHealth, etc.) are registered before
  // prefab deserialization tries to look them up.
  Engine::Resources::load_all_prefabs();

  // Start parsing the gameplay level on a worker thread. ECS/resource finalization
  // will happen later on the main thread while the menu is already running.
  Log::info("[EntryPoint] Requesting async preload for gameplay level: {}", PRELOADED_GAMEPLAY_LEVEL);
  Engine::LevelManager::preload_level_async(PRELOADED_GAMEPLAY_LEVEL);

#ifndef GAME_BUILD
  // ── Start source file watcher ───────────────────────────────────────
  // Watches Engine/GameLogic/ for .cpp/.h changes and auto-rebuilds
  // GameLogic.dll. The main-loop's check_and_reload() handles the reload.
  Engine::HotReload::start_source_watcher();

  // ── Background rebuild in case the DLL was outdated ─────────────────
  // The DLL was loaded above so the engine can start immediately.
  // This async build will produce a fresh DLL; check_and_reload()
  // will detect the updated timestamp and hot-swap it in.
  // Engine::HotReload::build_game_logic_async();

  // ^^ now handled by post build event
#endif

  // ── Run main game loop ──────────────────────────────────────────────
  Engine::Core::run("MainMenuState");

  // ── Cleanup ─────────────────────────────────────────────────────────
  // Core::exit() must run BEFORE unloading the DLL because scenes
  // (game states) are classes defined in GameLogic.dll — their vtables
  // become invalid once the DLL is freed.
#ifndef GAME_BUILD
  Engine::HotReload::stop_build();
  Engine::HotReload::stop_source_watcher();
#endif
  Engine::Core::exit();
  Engine::HotReload::unload_game_logic();

  // Delete all intentionally-leaked singletons now that the DLL is unloaded
  // and no code will access them. This frees the struct shells AND their
  // remaining STL structural overhead (sentinel nodes, iterator-debug proxies).
  Engine::ECS::destroy_world();
  Engine::ECS::ComponentTypeManager::destroy_type_map();
  Engine::EventManager::destroy_all();
  Engine::FunctionRegistry::destroy_instance();
  Engine::StateMachineBlueprintRegistry::destroy_instance();
}

#ifdef GAME_BUILD

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInst, [[maybe_unused]] HINSTANCE hInstPrev, [[maybe_unused]] PSTR cmdline, [[maybe_unused]] int cmdshow)
{
  RunGame();
  return 0;
}

#else

int main()
{
  RunGame();
  return 0;
}

#endif

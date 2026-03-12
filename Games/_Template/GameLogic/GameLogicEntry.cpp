#include "pch.h"
#include "MonkeyEngine/HotReloadManager.h"
#include "GameStates/EmptyState.h"

// =============================================================================
// Scene and system registrations
// =============================================================================
static void register_scenes()
{
    Engine::SceneManager::register_scene<EmptyState>("EmptyState");
}

// =============================================================================
// DLL Exports
// =============================================================================
extern "C" __declspec(dllexport) void RegisterGameLogic()
{
    Log::info("[GameLogic] Registering game logic...");
    register_scenes();
    Log::info("[GameLogic] Game logic registered successfully");
}

extern "C" __declspec(dllexport) void UnregisterGameLogic()
{
    Log::info("[GameLogic] Unregistering game logic...");

    Engine::SceneManager::scenes.clear();
    Engine::SceneManager::scene_order.clear();
    Engine::SceneManager::scene_stack.clear();
    Engine::SceneManager::current_scene = nullptr;

    Engine::StateMachineBlueprintRegistry::instance().clear_all_blueprints();
    Engine::FunctionRegistry::instance().clear_all();

    Log::info("[GameLogic] Game logic unregistered");
}

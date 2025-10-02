//-----------------------------------------------------------------------------------
// file:    LevelLoaderTestState.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a test state for the LevelLoader system. Provides
//          interactive controls for loading, spawning, reloading, and unloading
//          levels to verify level management functionality.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "StateManager.h"
#include "Core.h"
#include "BackgroundSystem.h"
#include "RenderSystem.h"
#include "BasicComponents.h"
#include <string>
#include "Logger.h"

class LevelLoaderTestState : public Engine::GameState<LevelLoaderTestState>
{
private:
  std::string currentLevelId;
  bool levelLoaded = false;
  ECS::EntityID backgroundEntityId;

public:
  void Load()
  {
  }

  void Init()
  {
    // Create background
    auto bg = Engine::WORLD.create_entity();
    bg.add_component<Components::Background>();
    bg.add_component<Components::Color>(0.2f, 0.2f, 0.3f, 1.0f);
    backgroundEntityId = bg.getId();

    // Add rendering systems
    Engine::WORLD.add_system<BackgroundSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();

    // Load the test level
    currentLevelId = Engine::LEVEL_LOADER.loadLevel("Assets/levels/test_level.json");

    if (!currentLevelId.empty())
    {
      levelLoaded = true;
      Log::info("Level loaded: " + currentLevelId);

      // Spawn the level
      if (Engine::LEVEL_LOADER.spawnLevel(currentLevelId))
      {
        Log::info("Level spawned successfully!");
      }
      else
      {
        Log::warning("Failed to spawn level");
      }
    }
    else
    {
      Log::warning("Failed to load level file");
    }
  }

  void Update()
  {
    // Press R to reload the level
    if (Engine::Input::IsKeyPressed(GLFW_KEY_R))
    {
      Log::info("Reloading level...");

      if (levelLoaded)
      {
        Engine::LEVEL_LOADER.unloadLevel(currentLevelId);
      }

      currentLevelId = Engine::LEVEL_LOADER.loadLevel("Assets/levels/test_level.json");
      if (!currentLevelId.empty())
      {
        levelLoaded = true;
        Engine::LEVEL_LOADER.spawnLevel(currentLevelId);
      }
    }

    // Press U to unload the level
    if (Engine::Input::IsKeyPressed(GLFW_KEY_U))
    {
      if (levelLoaded)
      {
        Log::info("Unloading level...");
        Engine::LEVEL_LOADER.unloadLevel(currentLevelId);
        levelLoaded = false;
        currentLevelId.clear();
      }
    }

    // Press L to spawn the level (if loaded but not spawned)
    if (Engine::Input::IsKeyPressed(GLFW_KEY_L))
    {
      if (levelLoaded && !currentLevelId.empty())
      {
        Log::info("Spawning level...");
        Engine::LEVEL_LOADER.spawnLevel(currentLevelId);
      }
    }
  }

  void Draw()
  {
    // Draw instructions
    ImGui::Begin("Level Loader Test");

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Level Loader Test State");
    ImGui::Separator();

    ImGui::Text("Current Level: %s", currentLevelId.empty() ? "None" : currentLevelId.c_str());
    ImGui::Text("Level Loaded: %s", levelLoaded ? "Yes" : "No");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Controls:");
    ImGui::BulletText("R - Reload Level");
    ImGui::BulletText("U - Unload Level");
    ImGui::BulletText("L - Spawn Level");

    ImGui::Spacing();
    ImGui::Separator();

    // Show entity count
    ImGui::Text("Active Entities: %d", Engine::WORLD.getEntityCount());

    // Camera controls
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Camera:");

    ImGui::End();
  }

  void Free()
  {
    Log::info("LevelLoaderTestState: Free");

    // Unload current level
    if (levelLoaded && !currentLevelId.empty())
    {
      Engine::LEVEL_LOADER.unloadLevel(currentLevelId);
    }

    // Destroy background entity
    Engine::WORLD.destroy_entity(backgroundEntityId);

    // Remove systems
    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
  }

  void Unload()
  {
    Log::info("LevelLoaderTestState: Unload");
  }
};
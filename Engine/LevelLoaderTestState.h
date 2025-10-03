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
  std::string current_level_id;
  bool level_loaded = false;
  ECS::EntityID background_entity_id = 0;

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
    background_entity_id = bg.get_id();

    // Add rendering systems
    Engine::WORLD.add_system<BackgroundSystem>();
    Engine::WORLD.add_system<StaticSpriteRenderSystem>();

    // Load the test level
    current_level_id = Engine::LEVEL_LOADER.load_level("Assets/levels/test_level.json");

    if (!current_level_id.empty())
    {
      level_loaded = true;
      Log::info("Level loaded: " + current_level_id);

      // Spawn the level
      if (Engine::LEVEL_LOADER.spawn_level(current_level_id))
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
    if (Engine::Input::is_key_pressed(GLFW_KEY_R))
    {
      Log::info("Reloading level...");

      if (level_loaded)
      {
        Engine::LEVEL_LOADER.unload_level(current_level_id);
      }

      current_level_id = Engine::LEVEL_LOADER.load_level("Assets/levels/test_level.json");
      if (!current_level_id.empty())
      {
        level_loaded = true;
        Engine::LEVEL_LOADER.spawn_level(current_level_id);
      }
    }

    // Press U to unload the level
    if (Engine::Input::is_key_pressed(GLFW_KEY_U))
    {
      if (level_loaded)
      {
        Log::info("Unloading level...");
        Engine::LEVEL_LOADER.unload_level(current_level_id);
        level_loaded = false;
        current_level_id.clear();
      }
    }

    // Press L to spawn the level (if loaded but not spawned)
    if (Engine::Input::is_key_pressed(GLFW_KEY_L))
    {
      if (level_loaded && !current_level_id.empty())
      {
        Log::info("Spawning level...");
        Engine::LEVEL_LOADER.spawn_level(current_level_id);
      }
    }
  }

  void Draw()
  {
    // Draw instructions
    ImGui::Begin("Level Loader Test");

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Level Loader Test State");
    ImGui::Separator();

    ImGui::Text("Current Level: %s", current_level_id.empty() ? "None" : current_level_id.c_str());
    ImGui::Text("Level Loaded: %s", level_loaded ? "Yes" : "No");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Controls:");
    ImGui::BulletText("R - Reload Level");
    ImGui::BulletText("U - Unload Level");
    ImGui::BulletText("L - Spawn Level");

    ImGui::Spacing();
    ImGui::Separator();

    // Show entity count
    ImGui::Text("Active Entities: %d", Engine::WORLD.get_entity_count());

    // Camera controls
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Camera:");

    ImGui::End();
  }

  void Free()
  {
    Log::info("LevelLoaderTestState: Free");

    // Unload current level
    if (level_loaded && !current_level_id.empty())
    {
      Engine::LEVEL_LOADER.unload_level(current_level_id);
    }

    // Destroy background entity
    Engine::WORLD.destroy_entity(background_entity_id);

    // Remove systems
    Engine::WORLD.remove_system<BackgroundSystem>();
    Engine::WORLD.remove_system<StaticSpriteRenderSystem>();
  }

  void Unload()
  {
    Log::info("LevelLoaderTestState: Unload");
  }
};
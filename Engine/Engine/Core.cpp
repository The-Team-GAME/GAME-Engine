//-----------------------------------------------------------------------------------
// file:    Core.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (50%)
//          Cheng Ming Xian (50%)

// email:   chew.s@digipen.edu, c.mingxian@digipen.edu
//
// brief:   This file implements the Core class which manages the main game loop.
//          Handles fixed timestep updates, rendering, input processing, and
//          coordinates all major engine subsystems.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Core.h"
#include "Precompiled.h"
#include <iostream>
#include "FileHandler.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include "Time.h"
#include "GUI/RegisterAllPanels.h"
#include "GUI/ViewportPanel.h"
#include "WindowManager.h"
#include "Perf.h"
#include "InputManager.h"
#include "Audio/AudioPlayer.h"
#include "Audio/AudioPlayerInternal.h"
#include "Lua/LuaManager.h"
#include "Resources.h"
#include "ECS/ECS.h"
#include "Events/WindowEvents.h"
#include "Systems/Systems.h"
#include "EventManager/EventManager.h"
#include "FileIO/Settings.h"
#include "FileIO/SettingsManager.h"
#include "Events/RegisterAllEvents.h"
#include "UndoManager/UndoManager.h"

namespace Engine
{
  // Static member definitions
  bool Core::is_running = false;
  f64 Core::accumulator{};
  s32 Core::step_limit = 3;
  bool Core::single_step_requested = false;

  /*=================================================================================
  Function Brief:
  Initializes all engine subsystems including window, graphics, and more.
  ==================================================================================*/
  void Core::init()
  {
    is_running = true;

    // Load engine settings
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::load("Assets/settings/engine_settings.json")) {
      Log::warning("Failed to load engine settings, using defaults");
    }

    // Load local settings
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::load("Assets/settings/local_settings.json")) {
      Log::warning("Failed to load local settings, using defaults");
    }

    // Project settings will be loaded below


    const auto& engine_settings = Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get();
    const auto& local_settings = Engine::FileIO::SettingsManager<Engine::FileIO::LocalSettings>::get();


    WindowManager::init(engine_settings.window_width, engine_settings.window_height, engine_settings.window_title);

    ImGuiManager::init();

    ImGuiManager::set_style(local_settings.imgui_theme);

    // Initialize graphics renderer
    if (!Engine::Graphics::init()) {
      throw std::runtime_error("Failed to initialize graphics renderer");
    }

    // Initialize resource manager
    if (!Engine::Resources::init("Assets")) { // TODO: move initial resource path to ProjectSettings
      throw std::runtime_error("Failed to initialize resource manager");
    }

    // Load all assets if enabled in settings
    if (engine_settings.load_all_assets_on_startup)
    {
      Engine::Resources::load_all_assets();
    }

    // Initialize audio manager
    if (!Engine::Audio::init()) {
      throw std::runtime_error("Failed to initialize audio manager");
    }

    // Initialize Lua manager
    if (!Engine::LuaManager::init()) {
      throw std::runtime_error("Failed to initialize Lua manager");
    }

    Engine::Graphics::set_viewport({ 0, 0, engine_settings.window_width, engine_settings.window_height });

    Engine::Events::register_all_events();

    // Load project settings
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::load("Assets/settings/project_settings.json")) {
      Log::warning("Failed to load project settings, using defaults");
    }

    // TODO: handle using custom reflector
    auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();
    for (auto& atlas_entry : project_settings.loaded_tilemaps) {
      if (!atlas_entry.texture_path.empty() && !atlas_entry.texture_handle) {
        atlas_entry.texture_handle = Engine::Resources::load_texture(atlas_entry.texture_path, true);
        if (!atlas_entry.texture_handle) {
          Log::warning("Failed to load texture for atlas: " + atlas_entry.texture_path);
        }
      }
    }


    // CORE INITIALIZATION

    // Initialize TilemapPanel to load saved tilemaps from project settings
    TilemapPanel::init();

    // Initialize ViewportPanel
    ViewportPanel::init();

    register_engine_systems();
    register_all_panels();

    // Just register one simple function
    Engine::LuaManager::register_function("log_message", [](const std::string& message) {
      std::cout << "[Lua] " << message << std::endl;
      });


    // Set basic globals
    Engine::LuaManager::set_global("GAME_VERSION", "1.0.0");
    Engine::LuaManager::set_global("ENGINE_NAME", "Monkey Engine");
    Engine::LuaManager::set_global("MAX_ENTITIES", 1000);

    // Load the script
    Engine::LuaManager::load_script("Assets/scripts/player_logic.lua");
    Engine::LuaManager::load_script("Assets/scripts/buttons_test.lua");
    Engine::LuaManager::load_script("Assets/scripts/behaviour_test.lua");
  }

  /*=================================================================================
  Function Brief:
  Main game loop that runs continuously until the window is closed. Implements
  fixed timestep updates with accumulator for consistent physics, handles input,
  updates all systems, and renders the scene. Supports physics step mode for debugging.

  Where to call it:
  Called once after Core construction to start the game loop.
  ==================================================================================*/
  void Core::run(std::string first_scene)
  {
    // Sync collision matrix from ProjectSettings to ECS internal collision matrix
    const auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get();
    auto& containers = Engine::ECS::Internal::get_containers();

    // Clear the collision matrix first (set all to false)
    for (auto& row : containers.collision_matrix) {
      row.reset();
    }

    // Rebuild collision matrix from name-based pairs
    for (const auto& [name_a, name_b] : project_settings.layer_collision_pairs) {
      Engine::ECS::LayerID layer_a = Engine::ECS::get_layer_id(name_a);
      Engine::ECS::LayerID layer_b = Engine::ECS::get_layer_id(name_b);

      // Only apply if both layers exist (have been registered)
      if (layer_a != Engine::ECS::INVALID_LAYER && layer_b != Engine::ECS::INVALID_LAYER) {
        containers.collision_matrix[layer_a].set(layer_b);
        containers.collision_matrix[layer_b].set(layer_a); // Symmetric
      }
    }

    Log::info("Loaded " + std::to_string(project_settings.layer_collision_pairs.size()) + " layer collision pairs from settings");

    SceneManager::load_scene(first_scene);

    while (!glfwWindowShouldClose(WindowManager::get_window()) && is_running)
    {
      // START OF TIME UPDATES ==============================================
      // Updates the real_dt and increments the accumulator
      Time::update_time();

      accumulator += Time::get_real_dt();


      if (accumulator > Time::get_unscaled_fixed_dt() * step_limit)
      {
        // Caps the accumulator so each gameloop will only go through 3 loops max
        accumulator = Time::get_unscaled_fixed_dt() * step_limit;
      }
      //std::cout << "DT: " << Time::get_real_dt() << " FPS: " << Time::calc_fps(Time::get_real_dt()) << std::endl;
      // END OF TIME UPDATES ================================================

      glfwPollEvents();

      // Update window title with FPS
      WindowManager::update_title();

      Engine::Audio::update();
      ImGuiManager::begin();

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while (accumulator >= Time::get_unscaled_fixed_dt())
      {
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_unscaled_fixed_dt();

        // Update input right before the main gameloop. This needs to be here because if it is out side the while loop,
        // World::update() won't have a chance to react before the input state is cleared on the next frame. 
        Input::update();

        if (Engine::Input::is_pressed(KeyCode::P))
        {
          Engine::Time::toggle_paused();
          std::cout << "Physics Step Mode: " << (Engine::Time::get_paused() ? "ON" : "OFF") << std::endl;
        }

        // Single-step when paused
        if (Engine::Input::is_pressed(KeyCode::RightBracket) && Engine::Time::get_paused())
        {
          single_step_requested = true;
        }

        // Undo last action
        if (Engine::Input::is_pressed(Engine::KeyCode::Z) && Engine::Input::is_held(Engine::KeyCode::CtrlLeft))
        {
          std::cout << "Undoing last action\n";
          Engine::UndoManager::undo();
        }

        // Redo last undone action
        if (Engine::Input::is_pressed(Engine::KeyCode::Y) && Engine::Input::is_held(Engine::KeyCode::CtrlLeft))
        {
          std::cout << "Redoing last undone action\n";
          Engine::UndoManager::redo();
        }

        // Delete selected entity (Delete key or Ctrl+D)
        if (Engine::Input::is_pressed(Engine::KeyCode::Delete))
        {
          // Find the entity with a Gizmo component (indicates it's selected)
          // TODO: This is a temporary solution; ideally, we should have a proper selection manager
          Engine::ECS::Entity selected_entity = Engine::ECS::get_first_entity_with<Components::Gizmo>();

          if (selected_entity != Engine::ECS::INVALID_ENTITY) {
            std::cout << "Deleting selected entity: " << selected_entity.get_id() << "\n";
            Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(selected_entity.get_id(), true);
          }
        }

        // Temporarily enable time for single-step
        if (single_step_requested)
        {
          Engine::Time::set_time_scale(1.0f);
        }

        // Main gameloop here
        Engine::SceneManager::update();
        Engine::ECS::update();

        // Always process ECS housekeeping (entity deletions, component modifications)
        // even when paused so entities can be created/destroyed in the editor
        Engine::ECS::process_housekeeping();

        // Restore paused state after single-step
        if (single_step_requested)
        {
          Engine::Time::set_time_scale(0.0f);
          single_step_requested = false;
        }

        TilemapPanel::update();
        ImGuiManager::update();

        // End of gameloop
      }

      // Rendering Start ========================================
      if (Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
        ViewportPanel::begin_render();
      }

      // Clear screen (automatically begins batch)
      // start of draw stuff
      Engine::SceneManager::draw();
      Engine::ECS::draw();
      TilemapPanel::draw_overlay();

      // end of draw stuff (flush remaining batches)
      Engine::Graphics::end_batch();

      if (Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
        ViewportPanel::end_render();
      }

      // Render ImGui (including viewport panel that shows the framebuffer)
      ImGuiManager::end();
      glfwSwapBuffers(WindowManager::get_window());
      // Rendering End ==========================================
    }

    // CORE CLEANUP
    unregister_engine_systems();
  }

  /*=================================================================================
  Function Brief:
  Cleans up all engine resources, shuts down subsystems
  ==================================================================================*/
  void Core::exit()
  {
    Engine::SceneManager::cleanup();
    //Engine::ECS::remove_event_listener<WindowResizeEvent>();
    ViewportPanel::exit();
    Engine::Audio::exit();
    Engine::Resources::exit();
    Engine::Graphics::exit();

    Engine::LuaManager::cleanup();
    ImGuiManager::exit();
    WindowManager::exit();
  }

  void Core::stop()
  {
    is_running = false;
  }
}

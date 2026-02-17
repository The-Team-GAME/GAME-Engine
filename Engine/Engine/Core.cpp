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
#include "Logger.h"
#include "FileHandler.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include "Time.h"
#include "GUI/RegisterAllPanels.h"
#include "GUI/ViewportPanel.h"
#include "GUI/FPSOverlay.h"
#include "WindowManager.h"
#include "Perf.h"
#include "InputManager.h"
#include "Audio/AudioPlayer.h"
#include "Audio/AudioPlayerInternal.h"
#include "Audio/AudioSystem.h"
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
#include "Easing.h"
#include "Lua/LuaComponentBindings.h"
#include "HotReloadManager.h"

namespace Engine
{
  // Static member definitions
  bool Core::is_running = false;
  f64 Core::accumulator{};
  s32 Core::step_limit = 3;
  bool Core::single_step_requested = false;
  static bool show_fps_overlay = false;

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

    // NOTE: Prefab/asset loading is deferred until after GameLogic.dll is loaded
    // (see EntryPoint.cpp) so that GameLogic components are registered first.

    // Initialize audio manager
    if (!Engine::Audio::init()) {
      throw std::runtime_error("Failed to initialize audio manager");
    }

    // Initialize FMOD Studio system
    if (!Engine::AudioSystem::init()) {
      throw std::runtime_error("Failed to initialize FMOD Studio audio system");
    }

    // Load FMOD sound banks
    Log::info("Loading FMOD sound banks...");
    Engine::AudioSystem::loadBank("Assets/sounds/Master.bank");
    Engine::AudioSystem::loadBank("Assets/sounds/Master.strings.bank");
    Engine::AudioSystem::loadBank("Assets/sounds/SFX.bank");
    Log::info("FMOD sound banks loaded successfully");

    // Load game settings (Window mode, Audio volumes)
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::load("Assets/settings/game_settings.json")) {
      Log::warning("Failed to load game settings, using defaults");
    }

    const auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get();

    // Apply window mode
    WindowManager::set_fullscreen(game_settings.fullscreen);

    // Apply VSync setting
    WindowManager::set_vsync(game_settings.vsync);

    // Apply audio volumes
    Engine::Audio::set_bgm_volume(game_settings.bgm_volume);
    Engine::Audio::set_sfx_volume(game_settings.sfx_volume);

    // Initialize Lua manager
    if (!Engine::LuaManager::init()) {
      throw std::runtime_error("Failed to initialize Lua manager");
    }

    Engine::LuaBindings::register_all_bindings(); // For lua bindings
    Engine::LuaManager::load_script("Assets/scripts/LazyMonkeyStateCallbacks.lua");

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
          Log::warning("Failed to load texture for atlas: {}", atlas_entry.texture_path);
        }
      }
    }


    // CORE INITIALIZATION

#ifndef GAME_BUILD
    // Initialize TilemapPanel to load saved tilemaps from project settings
    TilemapPanel::init();

    // Initialize ViewportPanel
    ViewportPanel::init();

    register_all_panels();
#endif
    register_engine_systems();
    Engine::Easing::register_all_easing_functions();

    // Just register one simple function
    Engine::LuaManager::register_function("log_message", [](const std::string& message) {
      Log::info("[Lua] {}", message);
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

    Log::info("Loaded {} layer collision pairs from settings", project_settings.layer_collision_pairs.size());

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
      // END OF TIME UPDATES ================================================

      glfwPollEvents();

      // Hot-reload check: detect GameLogic.dll changes and reload
#ifndef GAME_BUILD
      Engine::HotReload::check_and_reload();
#endif

      // Update window title with FPS
      WindowManager::update_title();

      Engine::Audio::update();
      Engine::AudioSystem::update();  // Update FMOD Studio system
      ImGuiManager::begin();

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while (accumulator >= Time::get_unscaled_fixed_dt())
      {
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_unscaled_fixed_dt();

        // Update input right before the main gameloop. This needs to be here because if it is out side the while loop,
        // World::update() won't have a chance to react before the input state is cleared on the next frame. 
        Input::update();

        if (Engine::Input::is_pressed(KeyCode::F10))
        {
          show_fps_overlay = !show_fps_overlay;
        }

        if (Engine::Input::is_pressed(KeyCode::F11))
        {
          auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
          settings.fullscreen = !settings.fullscreen;
          Engine::WindowManager::set_fullscreen(settings.fullscreen);
          Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
        }

#ifndef GAME_BUILD
        // Manual hot-reload: F5 triggers build + reload even when auto-reload is off
        if (Engine::Input::is_pressed(KeyCode::F5))
        {
          Engine::HotReload::request_manual_reload();
        }
#endif

        if (Engine::Input::is_pressed(KeyCode::P))
        {
          Engine::Time::toggle_paused();
          Log::info("Physics Step Mode: {}", (Engine::Time::get_paused() ? "ON" : "OFF"));
        }

        // FMOD Test: Press H to play hurt sound
        if (Engine::Input::is_pressed(KeyCode::H))
        {
          Engine::AudioSystem::playEvent("event:/SFX/MonkeyHurt");
          Log::info("Playing FMOD event: event:/SFX/MonkeyHurt");
        }

        // Single-step when paused
        if (Engine::Input::is_pressed(KeyCode::RightBracket) && Engine::Time::get_paused())
        {
          single_step_requested = true;
        }

        // Undo last action
        if (Engine::Input::is_pressed(Engine::KeyCode::Z) && Engine::Input::is_held(Engine::KeyCode::CtrlLeft))
        {
          Log::info("Undoing last action");
          Engine::UndoManager::undo();
        }

        // Redo last undone action
        if (Engine::Input::is_pressed(Engine::KeyCode::Y) && Engine::Input::is_held(Engine::KeyCode::CtrlLeft))
        {
          Log::info("Redoing last undone action");
          Engine::UndoManager::redo();
        }

        // Delete selected entity (Delete key or Ctrl+D)
        if (Engine::Input::is_pressed(Engine::KeyCode::Delete))
        {
          // Find the entity with a Gizmo component (indicates it's selected)
          // TODO: This is a temporary solution; ideally, we should have a proper selection manager
          Engine::ECS::Entity selected_entity = Engine::ECS::get_first_entity_with<Components::Gizmo>();

          if (selected_entity != Engine::ECS::INVALID_ENTITY) {
            Log::info("Deleting selected entity: {}", selected_entity.get_id());
            Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(selected_entity.get_id(), true);
          }
        }

        // Temporarily enable time for single-step
        if (single_step_requested)
        {
          Engine::Time::set_time_scale(1.0f);
        }


        // Two-phase ECS update:
        // 1. Game systems only run when not paused (or during single-step)
        if (!Engine::Time::get_paused() || single_step_requested)
        {
          Engine::SceneManager::update(); // LEgacy: Scene update
          Engine::ECS::update_game();
        }

        // 2. Editor systems always run (gizmo, camera control, etc.)
        Engine::ECS::update_editor();

        // Always process ECS housekeeping (entity deletions, component modifications)
        // even when paused so entities can be created/destroyed in the editor
        Engine::ECS::process_housekeeping();

        // Restore paused state after single-step
        if (single_step_requested)
        {
          Engine::Time::set_time_scale(0.0f);
          single_step_requested = false;
        }

#ifndef GAME_BUILD
        TilemapPanel::update();
#endif
        ImGuiManager::update();

        // End of gameloop
      }

      // Rendering Start ========================================
#ifndef GAME_BUILD
      if (Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
        ViewportPanel::begin_render();
      }
#endif

      // start of draw stuff

      // Clear screen (automatically begins batch)
      Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

      Engine::SceneManager::draw();
      Engine::ECS::draw();
#ifndef GAME_BUILD
      TilemapPanel::draw_overlay();
#endif

      if (show_fps_overlay)
      {
        Engine::GUI::RenderFPSOverlay();
      }

      // end of draw stuff (flush remaining batches)
      Engine::Graphics::end_batch();

#ifndef GAME_BUILD
      if (Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get().panel_viewport_mode) {
        ViewportPanel::end_render();
      }
#endif

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
#ifndef GAME_BUILD
    ViewportPanel::exit();
#endif
    Engine::Audio::exit();
    Engine::AudioSystem::shutdown();  // Shutdown FMOD Studio system
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

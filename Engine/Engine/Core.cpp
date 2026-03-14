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
#include "GUI/TilemapPanel.h"
#include "UpgradeManager.h"
#include "UpgradeUIStateMachine.h"
#include "Firebase/FirebaseManager.h"
#include "Firebase/FirebaseAuth.h"
#include "Firebase/FirebaseLuaBindings.h"

namespace Engine
{
  // Static member definitions
  bool Core::is_running = false;
  f64 Core::accumulator{};
  s32 Core::step_limit = 3;
  bool Core::single_step_requested = false;
  static bool show_fps_overlay = false;
  static std::optional<Engine::ECS::Prefab> entity_clipboard;

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

    // Initialize post-processing pipeline
    if (!Engine::Graphics::init_post_processing()) {
      Log::warning("Failed to initialize post-processing, effects will be disabled");
    }

    // Initialize resource manager
    if (!Engine::Resources::init("Assets")) { // TODO: move initial resource path to ProjectSettings
      throw std::runtime_error("Failed to initialize resource manager");
    }

    // NOTE: Prefab/asset loading is deferred until after GameLogic.dll is loaded
    // (see EntryPoint.cpp) so that GameLogic components are registered first.

    // Initialize FMOD Studio system
    if (!Engine::AudioSystem::init()) {
      throw std::runtime_error("Failed to initialize FMOD Studio audio system");
    }

    // Load FMOD sound banks
    Log::info("Loading FMOD sound banks...");
    Engine::AudioSystem::loadBank("Assets/fmodstudio/Build/Desktop/Master.bank");
    Engine::AudioSystem::loadBank("Assets/fmodstudio/Build/Desktop/Master.strings.bank");
    Engine::AudioSystem::loadBank("Assets/fmodstudio/Build/Desktop/SFX.bank");
    Engine::AudioSystem::loadBank("Assets/fmodstudio/Build/Desktop/SFXAmbient.bank");
    Engine::AudioSystem::loadBank("Assets/fmodstudio/Build/Desktop/SFXUI.bank");
    Engine::AudioSystem::loadBank("Assets/fmodstudio/Build/Desktop/BGM.bank");
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

    // Apply audio volumes via FMOD Studio buses
    Engine::AudioSystem::setBGMVolume(game_settings.bgm_volume);
    Engine::AudioSystem::setSFXVolume(game_settings.sfx_volume);

    // Initialize Firebase Cloud Firestore
    if (!Engine::Firebase::init()) {
      Log::warning("Firebase initialization failed, cloud features disabled");
    }
    Engine::Firebase::Auth::init();

    // Initialize Lua manager
    if (!Engine::LuaManager::init()) {
      throw std::runtime_error("Failed to initialize Lua manager");
    }

    Engine::Firebase::LuaBindings::register_bindings(); // Firebase "Cloud" table for Lua
    Engine::LuaBindings::register_all_bindings(); // For lua bindings
    Engine::LuaManager::load_script("Assets/scripts/LazyMonkeyStateCallbacks.lua");

    Engine::Graphics::set_viewport({ 0, 0, engine_settings.window_width, engine_settings.window_height });

    Engine::Events::register_all_events();

    // Load project settings
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::load("Assets/settings/project_settings.json")) {
      Log::warning("Failed to load project settings, using defaults");
    }

    // Load upgrade settings (roguelike upgrade system definitions)
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::UpgradeSettings>::load("Assets/settings/upgrade_settings.json")) {
      Log::warning("Failed to load upgrade settings, using defaults");
    }

    // Load card animation settings (upgrade picker UI effect parameters)
    if (!Engine::FileIO::SettingsManager<Engine::FileIO::CardAnimSettings>::load("Assets/settings/card_anim_settings.json")) {
      Log::warning("Failed to load card animation settings, using defaults");
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

      Engine::AudioSystem::update();  // Update FMOD Studio system
      Engine::Firebase::update();     // Poll async HTTP futures, dispatch callbacks
      Engine::Firebase::Auth::update(); // Check token expiry, refresh if needed
      ImGuiManager::begin();

      // Only runs the main gameloop if the accumulator is more than the fixed dt and limits the number of loops to loop_cap
      while (accumulator >= Time::get_unscaled_fixed_dt())
      {
        // To decrement the accumulator when each gameloop runs
        accumulator -= Time::get_unscaled_fixed_dt();

        // Update input right before the main gameloop. This needs to be here because if it is out side the while loop,
        // World::update() won't have a chance to react before the input state is cleared on the next frame. 
        Input::update();

        if (Engine::Input::is_pressed_raw(KeyCode::F10))
        {
          show_fps_overlay = !show_fps_overlay;
        }

        if (Engine::Input::is_pressed_raw(KeyCode::F11))
        {
          auto& settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
          settings.fullscreen = !settings.fullscreen;
          Engine::WindowManager::set_fullscreen(settings.fullscreen);
          Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
        }

        // Toggle post-processing on/off (F9)
        if (Engine::Input::is_pressed_raw(KeyCode::F9))
        {
          auto& pp = Engine::Graphics::get_post_process_settings();
          pp.enabled = !pp.enabled;
          Log::info("Post-Processing: {}", pp.enabled ? "ON" : "OFF");
        }

#ifndef GAME_BUILD
        // Manual hot-reload: F5 triggers build + reload even when auto-reload is off
        if (Engine::Input::is_pressed_raw(KeyCode::F5))
        {
          Engine::HotReload::request_manual_reload();
        }
#endif

#ifndef GAME_BUILD
        if (Engine::Input::is_pressed(KeyCode::P))
        {
          Engine::Time::toggle_paused();
          Log::info("Physics Step Mode: {}", (Engine::Time::get_paused() ? "ON" : "OFF"));
        }
#endif

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
        if (Engine::Input::is_pressed_raw(Engine::KeyCode::Z) && Engine::Input::is_held_raw(Engine::KeyCode::CtrlLeft))
        {
          Log::info("Undoing last action");
          Engine::UndoManager::undo();
        }

        // Redo last undone action
        if (Engine::Input::is_pressed_raw(Engine::KeyCode::Y) && Engine::Input::is_held_raw(Engine::KeyCode::CtrlLeft))
        {
          Log::info("Redoing last undone action");
          Engine::UndoManager::redo();
        }

#ifndef GAME_BUILD
        // Copy selected entity (Ctrl+C) - skip if tilemap tile selection is active
        if (Engine::Input::is_pressed_raw(Engine::KeyCode::C) && Engine::Input::is_held_raw(Engine::KeyCode::CtrlLeft))
        {
          if (!TilemapPanel::has_world_tile_selection()) {
            Engine::ECS::Entity selected = Engine::ECS::get_first_entity_with<Components::Gizmo>();
            if (selected.is_valid()) {
              entity_clipboard = Engine::ECS::entity_to_prefab(selected);
              Log::info("Copied entity {} to clipboard", selected.get_id());
            }
          }
        }

        // Paste copied entity (Ctrl+V) - skip if tilemap mode is handling paste
        if (Engine::Input::is_pressed_raw(Engine::KeyCode::V) && Engine::Input::is_held_raw(Engine::KeyCode::CtrlLeft))
        {
          if (entity_clipboard.has_value() && !TilemapPanel::is_tilemap_mode_enabled()) {
            Engine::ECS::Entity pasted = Engine::ECS::instantiate_prefab(entity_clipboard.value());
            Engine::ECS::EntityID pasted_id = pasted.get_id();
            Engine::ECS::Prefab pasted_prefab = entity_clipboard.value();

            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(pasted_id);

            Engine::UndoManager::record(
              // Do: Recreate the pasted entity
              [pasted_id, pasted_prefab]() {
                Engine::ECS::instantiate_prefab_with_id(pasted_prefab, pasted_id);
                Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(pasted_id);
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              },
              // Undo: Delete the pasted entity
              [pasted_id]() {
                Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(pasted_id, false);
                Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
              }
            );

            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
            Log::info("Pasted entity from clipboard as {}", pasted_id);
          }
        }
#endif

        // Delete selected entity (Delete key or Ctrl+D)
        if (Engine::Input::is_pressed_raw(Engine::KeyCode::Delete))
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

      // Clear the full window to black so letterbox/pillarbox bars are visible
      Engine::Graphics::clear_letterbox_bars(
        Engine::WindowManager::get_width(),
        Engine::WindowManager::get_height()
      );

      // Begin post-processing scene capture (redirects to PP FBO if enabled)
      Engine::Graphics::begin_post_processing();

      // Clear screen (automatically begins batch)
      Engine::Graphics::clear_screen({ 0.0f, 0.0f, 0.0f, 1.0f });

      Engine::SceneManager::draw();
      Engine::ECS::draw();

      // Flush batches before applying post-processing
      Engine::Graphics::end_batch();

      // Apply post-processing effects (bloom, LUT, gamma) and blit to current target
      Engine::Graphics::end_post_processing();

      // Begin new batch for unaffected overlays (these bypass post-processing)
      Engine::Graphics::begin_batch();

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
    // Clear undo/redo history and entity clipboard before subsystem shutdown.
    // Undo actions capture Prefab objects that hold ComponentArray pointers;
    // destroying them after DLL unload crashes because the vtable is gone.
    Engine::UndoManager::clear();
    entity_clipboard.reset();

    Engine::SceneManager::cleanup();

    // Destroy upgrade UI state machine before entities are wiped
    Engine::UpgradeUIStateMachine::destroy_instance();
    Engine::UpgradeManager::destroy_instance();

    // Force-destroy any entities that survived scene teardown so that
    // component destructors (including StateMachine callback name strings
    // and cached std::function objects) are freed before the CRT leak check.
    Engine::ECS::destroy_all_entities();
    Engine::ECS::process_housekeeping();

    // Clear WorldContainers contents (component arrays, entity data, systems, etc.).
    // The singleton itself is deleted later by destroy_world() after DLL unload.
    Engine::ECS::clear_world();

    // Release event handlers and queued events — their lambda closures may hold
    // resources and must be freed before the CRT leak check fires.
    Engine::EventManager::clear_all_listeners();
    //Engine::ECS::remove_event_listener<WindowResizeEvent>();
#ifndef GAME_BUILD
    ViewportPanel::exit();
#endif
    Engine::Firebase::Auth::shutdown();
    Engine::Firebase::shutdown();   // Shutdown Firebase / WinHTTP
    Engine::AudioSystem::shutdown();  // Shutdown FMOD Studio system
    Engine::Resources::exit();
    Engine::Graphics::exit_post_processing();
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

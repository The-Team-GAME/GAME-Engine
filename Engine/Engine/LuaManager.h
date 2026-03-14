//-----------------------------------------------------------------------------------
// file:    LuaManager.h
// project: Monkey Stole My GPA
//
// brief:   Singleton that owns the Lua/Sol2 runtime. Responsibilities:
//            - Create and configure the sol::state
//            - Drain LuaBindingRegistry (auto-bind every reflected component)
//            - Build the Entity __index metamethod (dot-syntax component access)
//            - Expose ECS, Input, Time, Audio, Log, SceneManager, LevelManager
//            - Load and execute scripts from Assets/scripts/
//            - Auto-register Lua functions into FunctionRegistry
//            - Load Lua-based state machine blueprints from Assets/scripts/blueprints/
//            - Event-driven hot-reload via ReadDirectoryChangesW when .lua files
//              are modified on disk
//
// Copyright (c) 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <sol/sol.hpp>
#include "EngineExport.h"
#include "ECS/Entity.h"

namespace Engine {

  class LuaManager {
  public:
    ENGINE_API static LuaManager& instance();
    ENGINE_API static void destroy_instance();

    /// Initialise the Lua runtime: open libs, bind all types, load scripts.
    /// Call AFTER the GameLogic DLL has been loaded (so all components are registered)
    /// and BEFORE load_all_prefabs().
    ENGINE_API void init();

    /// Shut down Lua and free the sol::state.
    ENGINE_API void shutdown();

    /// Access the raw sol::state (for advanced use / tests).
    ENGINE_API sol::state& lua();

    /// Hot-reload all Lua scripts and blueprints.
    /// Clears Lua-registered functions from FunctionRegistry, re-executes all
    /// scripts, and re-registers functions and blueprints.
    /// Safe to call at any time (e.g. from file watcher or editor button).
    ENGINE_API void reload_scripts();

    /// Check if the background file watcher detected changes and reload if so.
    /// Call once per frame from the main game loop. Lightweight — just checks
    /// an atomic flag set by the watcher thread. The actual reload happens on
    /// the main thread (safe for Lua/FunctionRegistry).
    ENGINE_API void check_and_reload_if_changed();

    /// Create a new Lua blueprint file from a template.
    /// Returns the full path to the created file, or empty string on failure.
    ENGINE_API std::string create_lua_blueprint(const std::string& blueprint_name);

    /// Delete a Lua blueprint file and unregister it.
    /// Returns true on success.
    ENGINE_API bool delete_lua_blueprint(const std::string& blueprint_name);

    /// Open a Lua blueprint file in the system's default editor.
    ENGINE_API void open_lua_blueprint_in_editor(const std::string& blueprint_name);

    /// Get the file path for a Lua blueprint by name.
    ENGINE_API std::string get_lua_blueprint_path(const std::string& blueprint_name) const;

    // ── Impl struct ─────────────────────────────────────────────────────
    // Public so that macros and LuaBindingRegistry getters can reference
    // the Getter type. The actual instance is private.
    struct Impl {
      using Getter = std::function<sol::object(sol::state_view, ECS::Entity)>;
      std::unordered_map<std::string, Getter> component_getters;
    };

  private:
    LuaManager();
    ~LuaManager();
    LuaManager(const LuaManager&) = delete;
    LuaManager& operator=(const LuaManager&) = delete;

    // ── Internal helpers called by init() ─────────────────────────────
    void drain_binding_registry();
    void bind_entity_type();
    void bind_ecs_table();
    void bind_log();
    void bind_input();
    void bind_time();
    void bind_audio();
    void bind_scene_manager();
    void bind_level_manager();
    void load_scripts();
    void load_lua_blueprints();

    // ── File watcher (ReadDirectoryChangesW background thread) ────────
    void start_file_watcher();
    void stop_file_watcher();
    static void file_watcher_thread_fn(LuaManager* self);

    // ── Data ──────────────────────────────────────────────────────────
    sol::state* lua_ = nullptr;
    Impl* impl_ = nullptr;

    // Tracks which function names were registered by Lua scripts (for cleanup on reload)
    std::unordered_set<std::string> lua_registered_functions_;

    // File watcher state
    std::atomic<bool> watcher_running_{ false };
    std::atomic<bool> lua_files_changed_{ false };
    std::thread watcher_thread_;
  };

} // namespace Engine

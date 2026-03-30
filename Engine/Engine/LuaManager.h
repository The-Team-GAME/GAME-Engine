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
//            - Bind register_blueprint() for inline state machine registration
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
#include "EngineExport.h"
#include "ECS/Entity.h"

// Forward-declare sol::state so callers of lua() can see the return type
// without pulling in the full 29K-line sol.hpp header.
namespace sol { class state; }

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

    /// Create a new Lua blueprint script from a template.
    /// Returns the full path to the created file, or empty string on failure.
    ENGINE_API std::string create_lua_blueprint(const std::string& blueprint_name);

    /// Delete a Lua blueprint script and unregister it.
    /// Returns true on success.
    ENGINE_API bool delete_lua_blueprint(const std::string& blueprint_name);

    /// Open a Lua blueprint script in the system's default editor.
    ENGINE_API void open_lua_blueprint_in_editor(const std::string& blueprint_name);

    /// Get the source file path for a Lua blueprint by name.
    ENGINE_API std::string get_lua_blueprint_path(const std::string& blueprint_name) const;

    /// Add a new state to an existing Lua blueprint.
    /// Generates stub callback functions and appends the state entry to the
    /// register_blueprint() call in the source file. Triggers hot-reload.
    ENGINE_API bool add_state_to_lua_blueprint(const std::string& bp_name,
                                               const std::string& state_name);

    /// Remove a state from an existing Lua blueprint.
    /// Removes the state entry from the register_blueprint() call but leaves
    /// callback functions intact (safer than deleting user code). Triggers hot-reload.
    ENGINE_API bool remove_state_from_lua_blueprint(const std::string& bp_name,
                                                    const std::string& state_name);

    // ── Impl struct ─────────────────────────────────────────────────────
    // Opaque implementation detail — defined in LuaManager.cpp.
    // Contains the component getter map that uses Sol2 types.
    struct Impl;

  private:
    LuaManager();
    ~LuaManager();
    LuaManager(const LuaManager&) = delete;
    LuaManager& operator=(const LuaManager&) = delete;

    // ── Internal helpers called by init() ─────────────────────────────
    void bind_vec2d_extras();      // Phase 1: full Vec2D before drain
    void drain_binding_registry();
    void bind_entity_type();       // enhanced in Phase 2 (has/remove/add)
    void bind_statemachine_methods(); // Phase 3
    void bind_animation_methods();    // Phase 3
    void bind_aabb_methods();         // Phase 3
    void bind_ecs_table();         // enhanced in Phase 6, 7
    void bind_physics();           // Phase 5
    void bind_resources();         // Phase 6
    void bind_register_blueprint();   // binds global register_blueprint() Lua function
    void bind_log();
    void bind_input();
    void bind_time();
    void bind_audio();
    void bind_settings();
    void bind_app();
    void bind_scene_manager();
    void bind_level_manager();
    void load_scripts();

    // ── Lua file editing helpers ──────────────────────────────────────
    static std::string to_snake_case(const std::string& name);
    static std::string to_lower(const std::string& s);

    // ── File watcher (ReadDirectoryChangesW background thread) ────────
    void start_file_watcher();
    void stop_file_watcher();
    static void file_watcher_thread_fn(LuaManager* self);

    // ── Data ──────────────────────────────────────────────────────────
    sol::state* lua_ = nullptr;
    Impl* impl_ = nullptr;

    // Tracks which function names were registered by Lua scripts (for cleanup on reload)
    std::unordered_set<std::string> lua_registered_functions_;

    // Tracks which file registered which blueprint (bp_name -> absolute file path)
    std::unordered_map<std::string, std::string> blueprint_source_files_;

    // Set by load_scripts() before each safe_script_file call so that
    // register_blueprint() can record the source file for each blueprint.
    std::string current_script_path_;

    // File watcher state
    std::atomic<bool> watcher_running_{ false };
    std::atomic<bool> lua_files_changed_{ false };
    std::thread watcher_thread_;
  };

} // namespace Engine

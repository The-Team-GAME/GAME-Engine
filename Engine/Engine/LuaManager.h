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
//
// Copyright (c) 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
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

    // ── Data ──────────────────────────────────────────────────────────
    sol::state* lua_ = nullptr;
    Impl* impl_ = nullptr;
  };

} // namespace Engine

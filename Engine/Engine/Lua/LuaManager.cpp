//-----------------------------------------------------------------------------------
// file:    LuaManager.cpp
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Implements the LuaManager namespace for managing the Lua scripting
//          environment. Handles initialization, cleanup, script loading, function
//          calls, global variable access, and C++/Lua bindings for the game engine.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------


#include "LuaManager.h"
#include "GameAPI.h"
#include "../Logger.h"
#include "../ECS/Entity.h"
#include <mutex>

namespace Engine
{
  namespace LuaManager
  {
    // Static variables for namespace
    static sol::state lua_state;
    static std::unordered_map<std::string, bool> loaded_scripts;
    static std::vector<std::function<void(sol::state&)>> binding_callbacks;
    static std::mutex bindings_mutex;

    bool init()
    {
      try {
        lua_state.open_libraries(sol::lib::base,
          sol::lib::math,
          sol::lib::package,
          sol::lib::string,
          sol::lib::table,
          sol::lib::io);

        // register high-level Game API
        Engine::GameAPI::register_api(lua_state);

        // run any registered binding callbacks (Game API / ECS bindings)
        run_bindings();

        Log::info("[LuaManager] Lua (sol2) initialized.");
        return true;
      }
      catch (const sol::error& e) {
        Log::error("[LuaManager] Failed to initialize: {}", e.what());
        return false;
      }
    }

    void cleanup()
    {
      loaded_scripts.clear();
      {
        std::lock_guard<std::mutex> lk(bindings_mutex);
        binding_callbacks.clear();
      }
      // Move the live Lua state into a local — this calls lua_close() on the old VM
      // immediately, freeing all Lua allocations NOW (before the CRT leak check fires).
      // Previously we used `lua_state = sol::state{}`, which created a fresh Lua VM
      // whose ~50 internal allocations were then reported as leaks by the CRT checker
      // (the static destructor that would have freed them runs after _CrtDumpMemoryLeaks).
      // After the move, lua_state holds a null lua_State*, so all guard checks below
      // (`if (!lua_state.lua_state())`) will correctly block further Lua calls.
      { sol::state closing = std::move(lua_state); }
      Log::info("[LuaManager] Lua closed.");
    }

    void load_script(const std::string& filename)
    {
      if (!lua_state.lua_state()) {
        Log::error("[LuaManager] Lua not initialized. Call init() first.");
        loaded_scripts[filename] = false;
        return;
      }

      try {
        lua_state.script_file(filename);
        loaded_scripts[filename] = true;
        Log::info("[LuaManager] Loaded script: {}", filename);
      }
      catch (const sol::error& e) {
        loaded_scripts[filename] = false;
        Log::error("[Lua Error] {}", e.what());
      }
    }

    void call_function(const std::string& name, float dt)
    {
      if (!lua_state.lua_state()) {
        Log::error("[LuaManager] Lua not initialized. Call init() first.");
        return;
      }

      try {
        sol::protected_function func = lua_state[name];
        if (func.valid()) {
          if (func.valid() && func.lua_state()) {
            // call with one float parameter
            sol::protected_function_result result = func(dt);
            if (!result.valid()) {
              sol::error err = result;
              Log::error("[Lua Error] {}", err.what());
            }
          }
        }
        else {
          // function not present; silent or log once
          // std::cerr << "[LuaManager] Function '" << name << "' not found.\n";
        }
      }
      catch (const sol::error& e) {
        Log::error("[Lua Error] {}", e.what());
      }
    }

    sol::state& get_state()
    {
      return lua_state;
    }

    const std::unordered_map<std::string, bool>& get_loaded_scripts()
    {
      return loaded_scripts;
    }

    void register_binding(std::function<void(sol::state&)> cb)
    {
      std::lock_guard<std::mutex> lk(bindings_mutex);
      binding_callbacks.emplace_back(std::move(cb));
    }

    void run_bindings()
    {
      std::vector<std::function<void(sol::state&)>> copy;
      {
        std::lock_guard<std::mutex> lk(bindings_mutex);
        copy = binding_callbacks;
      }
      for (auto& cb : copy) {
        try {
          cb(lua_state);
        }
        catch (const std::exception& e) {
          Log::error("[LuaManager] binding callback error: {}", e.what());
        }
      }
    }

    bool try_call_state_function(const std::string& func_name, Engine::ECS::Entity entity)
    {
      if (!lua_state.lua_state()) {
        return false;
      }

      try {
        sol::protected_function func = lua_state[func_name];
        if (!func.valid()) {
          return false; // Function doesn't exist in Lua
        }

        sol::protected_function_result result = func(&entity);
        if (!result.valid()) {
          //sol::error err = result;
          //std::cerr << "[Lua Error in " << func_name << "] " << err.what() << std::endl;
          return false;
        }

        return true; // Successfully called Lua function
      }
      catch ([[maybe_unused]] const sol::error& e) {
        //std::cerr << "[Lua Error] " << e.what() << std::endl;
        return false;
      }
    }
  }
}
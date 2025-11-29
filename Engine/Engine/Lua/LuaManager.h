//-----------------------------------------------------------------------------------
// file:    LuaManager.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   hongchinryan.how@digipen.edu
//
// brief:   Provides a singleton-style interface for managing the Lua scripting
//          environment. Handles initialization, cleanup, script loading, function
//          calls, global variable access, and C++/Lua bindings for the game engine.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

// I used https://www.lua.org/docs.html to understand the different functions on how to load
// and run different scripts and functions in Lua from C++.

// I also used the Sol2 documentation https://sol2.readthedocs.io/en/latest/ for reference on how to use Sol2 to bind C++ functions
// and types to Lua.

#pragma once
#include "../TypeDef.h"
#include "../Logger.h"
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace Engine
{
  namespace ECS { class Entity; } // Forward declaration

  namespace LuaManager
  {
    /**
     * @brief Initializes the Lua scripting environment and runs registered bindings.
     * @return True if initialization succeeded, false otherwise.
     */
    bool init();

    /**
     * @brief Cleans up the Lua environment and releases resources.
     */
    void cleanup();

    /**
     * @brief Loads and executes a Lua script file.
     * @param filename Path to the Lua script file.
     */
    void load_script(const std::string& filename);

    /**
     * @brief Calls a global Lua function by name, optionally passing delta time.
     * @param name Name of the Lua function to call.
     * @param dt Optional delta time parameter (default: 0.0f).
     */
    void call_function(const std::string& name, f32 dt = 0.0f);

    /**
     * @brief Gets a reference to the underlying sol::state Lua interpreter.
     * @return Reference to the sol::state object.
     */
    sol::state& get_state();

    /**
     * @brief Gets a map of loaded script filenames and their load status.
     * @return Unordered map of script filenames to boolean loaded status.
     */
    const std::unordered_map<std::string, bool>& get_loaded_scripts();

    /**
     * @brief Registers a C++ function into the Lua environment.
     * @tparam F Function type.
     * @param name Name to expose the function as in Lua.
     * @param func Function object or pointer.
     */
    template<typename F>
    void register_function(const std::string& name, F&& func);

    /**
     * @brief Sets a global variable in the Lua environment.
     * @tparam T Variable type.
     * @param name Name of the global variable.
     * @param value Value to set.
     */
    template<typename T>
    void set_global(const std::string& name, T&& value);

    /**
     * @brief Gets a global variable from the Lua environment.
     * @tparam T Variable type.
     * @param name Name of the global variable.
     * @param default_value Value to return if the variable is not found or conversion fails.
     * @return Value of the global variable or default_value.
     */
    template<typename T>
    T get_global(const std::string& name, T&& default_value = T{});

    /**
     * @brief Registers a binding callback to be run during initialization.
     * @param cb Callback function taking a sol::state&.
     */
    void register_binding(std::function<void(sol::state&)> cb);

    /**
     * @brief Runs all registered binding callbacks.
     */
    void run_bindings();

    /**
     * @brief Calls a Lua state callback function if it exists.
     * @param func_name Name of the Lua function to call.
     * @param entity Entity to pass to the Lua function.
     * @return True if the Lua function was called successfully, false if not found or error.
     */
    bool try_call_state_function(const std::string& func_name, Engine::ECS::Entity& entity);

    /**
     * @brief Automatically binds a component type to Lua using reflection metadata.
     * @tparam T Component type with reflection data.
     */
    template<typename T>
    void bind_component_type();

    /**
     * @brief Binds multiple component types at once using variadic templates.
     * @tparam Components Pack of component types to bind.
     */
    template<typename... Components>
    void bind_all_components();
  }
}

// Template implementations must be in header
namespace Engine
{
  namespace LuaManager
  {
    /**
     * @brief Registers a C++ function into the Lua environment.
     * @tparam F Function type.
     * @param name Name to expose the function as in Lua.
     * @param func Function object or pointer.
     */
    template<typename F>
    void register_function(const std::string& name, F&& func) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        Log::error("[LuaManager] Lua not initialized. Call init() first.");
        return;
      }
      lua.set_function(name, std::forward<F>(func));
    }

    /**
     * @brief Sets a global variable in the Lua environment.
     * @tparam T Variable type.
     * @param name Name of the global variable.
     * @param value Value to set.
     */
    template<typename T>
    void set_global(const std::string& name, T&& value) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        Log::error("[LuaManager] Lua not initialized. Call init() first.");
        return;
      }
      lua[name] = std::forward<T>(value);
    }

    /**
     * @brief Gets a global variable from the Lua environment.
     * @tparam T Variable type.
     * @param name Name of the global variable.
     * @param default_value Value to return if the variable is not found or conversion fails.
     * @return Value of the global variable or default_value.
     */
    template<typename T>
    T get_global(const std::string& name, T&& default_value) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        Log::error("[LuaManager] Lua not initialized. Call init() first.");
        return default_value;
      }
      sol::object obj = lua[name];
      if (!obj.valid()) return default_value;
      try {
        return obj.as<T>();
      } catch (...) {
        return default_value;
      }
    }

    /**
     * @brief Automatically binds a component type to Lua using reflection metadata.
     * Note: This is a template declaration. Actual implementation will use your reflection system.
     */
    template<typename T>
    void bind_component_type() {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
        return;
      }

      // This will be implemented in LuaComponentBindings.h with your reflection system
      // For now, this is a placeholder that will be specialized
      std::cerr << "[LuaManager] bind_component_type called but not specialized for this type\n";
    }

    /**
     * @brief Binds multiple component types at once using variadic templates.
     * @tparam Components Pack of component types to bind.
     */
    template<typename... Components>
    void bind_all_components() {
      (bind_component_type<Components>(), ...);
    }
  }
}
#pragma once
#include <sol/sol.hpp>
#include <include/lua.hpp>
#include <string>
#include <unordered_map>

namespace Engine
{
  namespace LuaManager
  {
    /*=================================================================================
    Function Brief:
    Initializes the Lua state and opens standard libraries. Must be called before
    using any other LuaManager functions.

    Returns:
    True if initialization succeeded, false otherwise.

    Where to call it:
    At engine startup, before loading any scripts or calling Lua functions.
    ==================================================================================*/
    bool init();

    /*=================================================================================
    Function Brief:
    Cleans up and closes the Lua state. Should be called at engine shutdown.

    Where to call it:
    At engine shutdown, after all Lua operations are complete.
    ==================================================================================*/
    void cleanup();

    /*=================================================================================
    Function Brief:
    Loads and executes a Lua script file. Handles errors gracefully with console output.

    Parameters:
    filename - Path to the .lua file to load and execute.

    Where to call it:
    During initialization or when you need to load new scripts at runtime.
    ==================================================================================*/
    void load_script(const std::string& filename);

    /*=================================================================================
    Function Brief:
    Calls a Lua function with an optional delta time parameter. Handles errors
    gracefully and validates function existence.

    Parameters:
    name - Name of the Lua function to call.
    dt   - Optional delta time parameter (default: 0.0f).

    Where to call it:
    In your game loop or event handlers when you need to execute Lua callbacks.
    ==================================================================================*/
    void call_function(const std::string& name, float dt = 0.0f);

    /*=================================================================================
    Function Brief:
    Registers a C++ function that can be called from Lua scripts. Uses modern
    C++ features like lambdas for easy binding.

    Parameters:
    name - The name Lua will use to call this function.
    func - Function object, lambda, or function pointer to register.

    Where to call it:
    During initialization, after init() but before loading scripts.
    ==================================================================================*/
    template<typename T>
    void register_function(const std::string& name, T&& func);

    /*=================================================================================
    Function Brief:
    Sets a global variable in the Lua state that scripts can access.

    Parameters:
    name  - Variable name in Lua.
    value - Value to assign to the variable.

    Where to call it:
    During initialization or when you need to pass data from C++ to Lua.
    ==================================================================================*/
    template<typename T>
    void set_global(const std::string& name, T&& value);

    /*=================================================================================
    Function Brief:
    Gets a global variable value from the Lua state.

    Parameters:
    name - Variable name to retrieve from Lua.

    Returns:
    The value cast to type T, or default-constructed T if variable doesn't exist.

    Where to call it:
    When you need to read data that Lua scripts have modified.
    ==================================================================================*/
    template<typename T>
    T get_global(const std::string& name);

    /*=================================================================================
    Function Brief:
    Gets direct access to the Sol2 state for advanced operations.

    Returns:
    Reference to the internal sol::state object.

    Where to call it:
    When you need to perform advanced Lua operations not covered by other functions.
    ==================================================================================*/
    sol::state& get_state();

    /*=================================================================================
    Function Brief:
    Gets a map of all currently loaded script names and their load status.

    Returns:
    Constant reference to a map of script names and whether they loaded successfully.
    ==================================================================================*/
    const std::unordered_map<std::string, bool>& get_loaded_scripts();
  }
}

// Template implementations must be in header
namespace Engine
{
  namespace LuaManager
  {
    template<typename T>
    void register_function(const std::string& name, T&& func) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
        return;
      }
      lua[name] = std::forward<T>(func);
      std::cout << "[LuaManager] Registered C++ function: " << name << std::endl;
    }

    template<typename T>
    void set_global(const std::string& name, T&& value) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
        return;
      }
      lua[name] = std::forward<T>(value);
    }

    template<typename T>
    T get_global(const std::string& name) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
        return T{};
      }
      return lua[name];
    }
  }
}
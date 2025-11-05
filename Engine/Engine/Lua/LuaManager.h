#pragma once
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace Engine
{
  namespace LuaManager
  {
    // lifecycle
    bool init();
    void cleanup();

    // scripts / calling
    void load_script(const std::string& filename);
    void call_function(const std::string& name, float dt = 0.0f);

    // access to sol state
    sol::state& get_state();

    // info
    const std::unordered_map<std::string, bool>& get_loaded_scripts();

    // register a C++ function into Lua (template - implemented below)
    template<typename F>
    void register_function(const std::string& name, F&& func);

    // set / get Lua globals (templates)
    template<typename T>
    void set_global(const std::string& name, T&& value);

    template<typename T>
    T get_global(const std::string& name, T&& default_value = T{});

    // allow registering binding callbacks (e.g. GameAPI::register_api)
    // Callbacks are invoked during init() automatically; you may also call run_bindings() manually
    void register_binding(std::function<void(sol::state&)> cb);
    void run_bindings();
  }
}

// Template implementations must be in header
namespace Engine
{
  namespace LuaManager
  {
    template<typename F>
    void register_function(const std::string& name, F&& func) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
        return;
      }
      lua.set_function(name, std::forward<F>(func));
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
    T get_global(const std::string& name, T&& default_value) {
      sol::state& lua = get_state();
      if (!lua.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
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
  }
}
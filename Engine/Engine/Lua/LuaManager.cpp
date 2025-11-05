#include "LuaManager.h"
#include "GameAPI.h"
#include <iostream>
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

        std::cout << "[LuaManager] Lua (sol2) initialized.\n";
        return true;
      }
      catch (const sol::error& e) {
        std::cerr << "[LuaManager] Failed to initialize: " << e.what() << "\n";
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
      lua_state = sol::state{};
      std::cout << "[LuaManager] Lua closed.\n";
    }

    void load_script(const std::string& filename)
    {
      if (!lua_state.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
        loaded_scripts[filename] = false;
        return;
      }

      try {
        lua_state.script_file(filename);
        loaded_scripts[filename] = true;
        std::cout << "[LuaManager] Loaded script: " << filename << std::endl;
      }
      catch (const sol::error& e) {
        loaded_scripts[filename] = false;
        std::cerr << "[Lua Error] " << e.what() << std::endl;
      }
    }

    void call_function(const std::string& name, float dt)
    {
      if (!lua_state.lua_state()) {
        std::cerr << "[LuaManager] Lua not initialized. Call init() first.\n";
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
              std::cerr << "[Lua Error] " << err.what() << std::endl;
            }
          }
        } else {
          // function not present; silent or log once
          // std::cerr << "[LuaManager] Function '" << name << "' not found.\n";
        }
      }
      catch (const sol::error& e) {
        std::cerr << "[Lua Error] " << e.what() << std::endl;
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
        } catch (const std::exception& e) {
          std::cerr << "[LuaManager] binding callback error: " << e.what() << std::endl;
        }
      }
    }
  }
}
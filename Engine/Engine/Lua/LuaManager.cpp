#include "LuaManager.h"
#include <iostream>

namespace Engine
{
  namespace LuaManager
  {
    // Static variables for namespace
    static sol::state lua_state;
    static std::unordered_map<std::string, bool> loaded_scripts;

    bool init()
    {
      try {
        lua_state.open_libraries(sol::lib::base, 
                                sol::lib::math, 
                                sol::lib::package,
                                sol::lib::string,
                                sol::lib::table,
                                sol::lib::io);
        
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
        sol::function func = lua_state[name];
        if (func.valid()) {
          func(dt);
        } else {
          std::cerr << "[LuaManager] Function '" << name << "' not found or not callable.\n";
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
  }
}
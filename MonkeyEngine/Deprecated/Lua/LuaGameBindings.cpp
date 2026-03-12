//-----------------------------------------------------------------------------------
// file:    LuaGameBindings.cpp
//
// brief:   Implementation of the game-specific Lua component binding registry.
//          Stores handler maps in the engine's address space so GameLogic DLLs
//          can register/unregister handlers across hot-reload cycles.
//-----------------------------------------------------------------------------------

#include "LuaGameBindings.h"
#include <unordered_map>

namespace Engine::LuaBindings
{
  // Static handler maps (engine-owned, survive DLL reload)
  static std::unordered_map<std::string, GetComponentFn>&    get_component_map()    { static std::unordered_map<std::string, GetComponentFn>    m; return m; }
  static std::unordered_map<std::string, HasComponentFn>&    has_component_map()    { static std::unordered_map<std::string, HasComponentFn>    m; return m; }
  static std::unordered_map<std::string, RemoveComponentFn>& remove_component_map() { static std::unordered_map<std::string, RemoveComponentFn> m; return m; }
  static std::unordered_map<std::string, GetFirstEntityFn>&  get_first_entity_map() { static std::unordered_map<std::string, GetFirstEntityFn>  m; return m; }

  void register_lua_get_component(const std::string& name, GetComponentFn fn)    { get_component_map()[name] = fn; }
  void register_lua_has_component(const std::string& name, HasComponentFn fn)    { has_component_map()[name] = fn; }
  void register_lua_remove_component(const std::string& name, RemoveComponentFn fn) { remove_component_map()[name] = fn; }
  void register_lua_get_first_entity(const std::string& name, GetFirstEntityFn fn)  { get_first_entity_map()[name] = fn; }

  void clear_lua_game_handlers()
  {
    get_component_map().clear();
    has_component_map().clear();
    remove_component_map().clear();
    get_first_entity_map().clear();
  }

  sol::object dispatch_get_component(ECS::Entity e, const std::string& name)
  {
    auto& m = get_component_map();
    auto it = m.find(name);
    if (it != m.end()) return it->second(e);
    return sol::object(LuaManager::get_state(), sol::nil);
  }

  bool dispatch_has_component(ECS::Entity e, const std::string& name)
  {
    auto& m = has_component_map();
    auto it = m.find(name);
    if (it != m.end()) return it->second(e);
    return false;
  }

  void dispatch_remove_component(ECS::Entity e, const std::string& name)
  {
    auto& m = remove_component_map();
    auto it = m.find(name);
    if (it != m.end()) it->second(e);
  }

  ECS::Entity dispatch_get_first_entity(const std::string& name)
  {
    auto& m = get_first_entity_map();
    auto it = m.find(name);
    if (it != m.end()) return it->second();
    return ECS::Entity{};
  }
}

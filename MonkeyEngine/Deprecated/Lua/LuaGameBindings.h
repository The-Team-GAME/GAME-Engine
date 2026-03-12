//-----------------------------------------------------------------------------------
// file:    LuaGameBindings.h
//
// brief:   Registry for game-specific Lua component bindings. Allows GameLogic
//          DLLs to register handlers for get_component, has_component,
//          remove_component, and get_first_entity_with without the engine
//          depending on game-specific component types.
//-----------------------------------------------------------------------------------
#pragma once

#include "../EngineExport.h"
#include "../ECS/Entity.h"
#include "LuaManager.h"
#include <string>

namespace Engine::LuaBindings
{
  // Handler function pointer types
  using GetComponentFn    = sol::object(*)(ECS::Entity);
  using HasComponentFn    = bool(*)(ECS::Entity);
  using RemoveComponentFn = void(*)(ECS::Entity);
  using GetFirstEntityFn  = ECS::Entity(*)();

  // Register game-specific Lua component handlers (called from GameLogic DLL)
  ENGINE_API void register_lua_get_component(const std::string& name, GetComponentFn fn);
  ENGINE_API void register_lua_has_component(const std::string& name, HasComponentFn fn);
  ENGINE_API void register_lua_remove_component(const std::string& name, RemoveComponentFn fn);
  ENGINE_API void register_lua_get_first_entity(const std::string& name, GetFirstEntityFn fn);

  // Clear all game-registered handlers (call from UnregisterGameLogic)
  ENGINE_API void clear_lua_game_handlers();

  // Dispatch functions used by LuaComponentBindings at Lua call time
  ENGINE_API sol::object dispatch_get_component(ECS::Entity e, const std::string& name);
  ENGINE_API bool        dispatch_has_component(ECS::Entity e, const std::string& name);
  ENGINE_API void        dispatch_remove_component(ECS::Entity e, const std::string& name);
  ENGINE_API ECS::Entity dispatch_get_first_entity(const std::string& name);
}

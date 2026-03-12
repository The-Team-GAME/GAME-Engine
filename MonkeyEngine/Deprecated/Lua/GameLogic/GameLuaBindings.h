//-----------------------------------------------------------------------------------
// file:    GameLuaBindings.h
//
// brief:   Registers game-specific component Lua handlers with the engine's
//          LuaGameBindings registry. Called during RegisterGameLogic() so that
//          Lua scripts can access game component types (Player, LazyMonkey, etc.)
//          via Entity:get_component / has_component / remove_component and
//          ECS.get_first_entity_with.
//-----------------------------------------------------------------------------------
#pragma once

#include "Components/GameplayComponent.h"
#include "MonkeyEngine/Lua/LuaGameBindings.h"

namespace GameLuaBindings
{
  // ── get_component handlers ──────────────────────────────────────
  inline sol::object get_lazy_monkey(Engine::ECS::Entity e) {
    auto& lua = Engine::LuaManager::get_state();
    auto* comp = e.get_component<Components::LazyMonkey>();
    return comp ? sol::make_object(lua, comp) : sol::object(lua, sol::nil);
  }

  inline sol::object get_player(Engine::ECS::Entity e) {
    auto& lua = Engine::LuaManager::get_state();
    auto* comp = e.get_component<Components::Player>();
    return comp ? sol::make_object(lua, comp) : sol::object(lua, sol::nil);
  }

  inline sol::object get_pending_knockback(Engine::ECS::Entity e) {
    auto& lua = Engine::LuaManager::get_state();
    auto* comp = e.get_component<Components::PendingKnockback>();
    return comp ? sol::make_object(lua, *comp) : sol::object(lua, sol::nil);
  }

  // ── has_component handlers ──────────────────────────────────────
  inline bool has_pending_knockback(Engine::ECS::Entity e) {
    return e.has_component<Components::PendingKnockback>();
  }

  // ── remove_component handlers ───────────────────────────────────
  inline void remove_pending_knockback(Engine::ECS::Entity e) {
    e.remove_component<Components::PendingKnockback>();
  }

  // ── get_first_entity_with handlers ──────────────────────────────
  inline Engine::ECS::Entity first_player() {
    return Engine::ECS::get_first_entity_with<Components::Player>();
  }

  inline Engine::ECS::Entity first_lazy_monkey() {
    return Engine::ECS::get_first_entity_with<Components::LazyMonkey>();
  }

  // ── Registration / cleanup ──────────────────────────────────────
  inline void register_all() {
    using namespace Engine::LuaBindings;

    register_lua_get_component("LazyMonkey",      &get_lazy_monkey);
    register_lua_get_component("Player",           &get_player);
    register_lua_get_component("PendingKnockback", &get_pending_knockback);

    register_lua_has_component("PendingKnockback", &has_pending_knockback);

    register_lua_remove_component("PendingKnockback", &remove_pending_knockback);

    register_lua_get_first_entity("Player",     &first_player);
    register_lua_get_first_entity("LazyMonkey", &first_lazy_monkey);
  }

  inline void unregister_all() {
    Engine::LuaBindings::clear_lua_game_handlers();
  }
}

//-----------------------------------------------------------------------------------
// file:    LuaBindingRegistry.h
// project: Monkey Stole My GPA
//
// brief:   Side registry that collects component-binding callbacks at static
//          initialization time. Each REGISTER_COMPONENT / REGISTER_COMPONENT_CUSTOM
//          macro pushes a lambda here. LuaManager::init() drains the list once,
//          after all component types have been registered.
//
//          This file includes sol/sol.hpp directly for type-safe binder and
//          getter signatures.
//
// Copyright (c) 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <vector>
#include <string>
#include <sol/sol.hpp>
#include "EngineExport.h"
#include "ECS/Entity.h"

namespace Engine {

  /// Side registry for deferred Lua component binders.
  /// LuaManager::init() calls every entry exactly once.
  class LuaBindingRegistry {
  public:
    /// Binder:  given a sol::state, register the component's usertype
    /// Getter:  given a sol::state_view + Entity, return the component as sol::object
    struct Entry {
      std::string short_name;                                              // e.g. "Transform2D"
      std::function<void(sol::state&)> binder;                             // binds the type
      std::function<sol::object(sol::state_view, ECS::Entity)> getter;     // gets component ptr
    };

    /// Method binder: augments an already-registered usertype with extra methods.
    /// Pushed by LUA_BIND_METHODS macros (typically from GameLogic).
    struct MethodBinderEntry {
      std::string short_name;                                              // e.g. "Health"
      std::function<void(sol::state&)> binder;                             // adds methods to existing usertype
    };

    ENGINE_API static LuaBindingRegistry& instance();

    ENGINE_API static void destroy_instance();

    /// Push a binding callback (called from REGISTER_COMPONENT macros at static init).
    void push(Entry entry) {
      entries_.push_back(std::move(entry));
    }

    /// Push a method binder (called from LUA_BIND_METHODS macros at static init).
    void push_method_binder(MethodBinderEntry entry) {
      method_binder_entries_.push_back(std::move(entry));
    }

    /// Access all pending entries. LuaManager drains this during init.
    const std::vector<Entry>& entries() const { return entries_; }

    /// Access all pending method binder entries. LuaManager drains this after entries.
    const std::vector<MethodBinderEntry>& method_binder_entries() const { return method_binder_entries_; }

  private:
    std::vector<Entry> entries_;
    std::vector<MethodBinderEntry> method_binder_entries_;

    LuaBindingRegistry() = default;
    LuaBindingRegistry(const LuaBindingRegistry&) = delete;
    LuaBindingRegistry& operator=(const LuaBindingRegistry&) = delete;
  };

} // namespace Engine

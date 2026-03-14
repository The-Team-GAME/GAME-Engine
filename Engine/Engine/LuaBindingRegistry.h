//-----------------------------------------------------------------------------------
// file:    LuaBindingRegistry.h
// project: Monkey Stole My GPA
//
// brief:   Side registry that collects component-binding callbacks at static
//          initialization time. Each REGISTER_COMPONENT / REGISTER_COMPONENT_CUSTOM
//          macro pushes a lambda here. LuaManager::init() drains the list once,
//          after all component types have been registered.
//
//          Now that Sol2 is in the precompiled header, this file uses Sol2 types
//          directly for type-safe binder and getter signatures.
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

    ENGINE_API static LuaBindingRegistry& instance();

    ENGINE_API static void destroy_instance();

    /// Push a binding callback (called from REGISTER_COMPONENT macros at static init).
    void push(Entry entry) {
      entries_.push_back(std::move(entry));
    }

    /// Access all pending entries. LuaManager drains this during init.
    const std::vector<Entry>& entries() const { return entries_; }

  private:
    std::vector<Entry> entries_;

    LuaBindingRegistry() = default;
    LuaBindingRegistry(const LuaBindingRegistry&) = delete;
    LuaBindingRegistry& operator=(const LuaBindingRegistry&) = delete;
  };

} // namespace Engine

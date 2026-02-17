//-----------------------------------------------------------------------------------
// file:    StateMachineBlueprintRegistry.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang
//
// email:   chew.s@digipen.edu
//
// brief:   Blueprint registry for state machines. Stores the "code truth" for
//          which states exist and what callbacks they use. On load, the engine
//          merges blueprint callbacks onto serialized prefab data so that
//          adding/removing states or callbacks in code auto-propagates to
//          existing entities without touching animation/audio prefab data.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "../Logger.h"
#include "../EngineExport.h"
#include "../FunctionRegistry.h"

namespace Engine
{

  // ============================================================================
  // State Blueprint — lightweight description of one state (callbacks only)
  // ============================================================================

  struct StateBlueprint {
    std::string state_name;
    std::string on_enter;
    std::string on_update;
    std::string on_exit;

    StateBlueprint() = default;

    StateBlueprint(const std::string& name,
      const std::string& enter = "",
      const std::string& update = "",
      const std::string& exit = "")
      : state_name(name)
      , on_enter(enter)
      , on_update(update)
      , on_exit(exit)
    {
    }
  };

  // ============================================================================
  // State Machine Blueprint — a list of state blueprints + initial state
  // ============================================================================

  struct StateMachineBlueprint {
    std::vector<StateBlueprint> states;
    std::string initial_state;

    StateMachineBlueprint() = default;

    StateMachineBlueprint(std::initializer_list<StateBlueprint> state_list,
      const std::string& initial = "")
      : states(state_list)
      , initial_state(initial)
    {
    }
  };

  // ============================================================================
  // Blueprint Registry — singleton mapping blueprint_id -> StateMachineBlueprint
  // ============================================================================

  class ENGINE_API StateMachineBlueprintRegistry {
  public:
    static StateMachineBlueprintRegistry& instance();

    /// Register a blueprint under a unique id
    void register_blueprint(const std::string& id, const StateMachineBlueprint& blueprint) {
      if (blueprints_.count(id)) {
        Log::warning("StateMachineBlueprintRegistry: Overwriting blueprint '{}'", id);
      }
      blueprints_[id] = blueprint;
    }

    /// Retrieve a blueprint by id (returns nullptr if not found)
    const StateMachineBlueprint* get_blueprint(const std::string& id) const {
      auto it = blueprints_.find(id);
      return (it != blueprints_.end()) ? &it->second : nullptr;
    }

    /// Check if a blueprint exists
    bool has_blueprint(const std::string& id) const {
      return blueprints_.count(id) > 0;
    }

    /// Get all registered blueprint ids (for editor/debug)
    std::vector<std::string> get_all_ids() const {
      std::vector<std::string> ids;
      ids.reserve(blueprints_.size());
      for (const auto& [id, _] : blueprints_) {
        ids.push_back(id);
      }
      return ids;
    }

  private:
    std::unordered_map<std::string, StateMachineBlueprint> blueprints_;

    StateMachineBlueprintRegistry() = default;
    StateMachineBlueprintRegistry(const StateMachineBlueprintRegistry&) = delete;
    StateMachineBlueprintRegistry& operator=(const StateMachineBlueprintRegistry&) = delete;
  };

} // namespace Engine

// ============================================================================
// Helpers for auto-registering callbacks within blueprint macros
// ============================================================================

namespace Engine::detail {

  /// Registers a function in FunctionRegistry and returns its string name.
  /// Overload for real function pointers.
  template<typename Func>
  inline std::string auto_reg(const char* name, Func func) {
    using Sig = typename Engine::function_traits<std::decay_t<decltype(func)>>::signature;
    Engine::FunctionRegistry::instance().register_function<Sig>(name, std::function<Sig>(func));
    return name;
  }

  /// Overload for nullptr (no callback).
  inline std::string auto_reg(const char*, std::nullptr_t) {
    return "";
  }

} // namespace Engine::detail

// ============================================================================
// Sentinel for "no callback" slots
// ============================================================================
#define NO_CALLBACK nullptr

// ============================================================================
// STATE macro — auto-registers each callback and pushes a StateBlueprint.
// Use inside REGISTER_STATE_MACHINE_BLUEPRINT.
//
// Usage:  STATE("Idle", MyCallbacks::idle_enter, MyCallbacks::idle_update, NO_CALLBACK)
// ============================================================================
#define STATE(STATE_NAME, ON_ENTER, ON_UPDATE, ON_EXIT) \
    bp.states.push_back(::Engine::StateBlueprint{ \
        STATE_NAME, \
        ::Engine::detail::auto_reg(#ON_ENTER, ON_ENTER), \
        ::Engine::detail::auto_reg(#ON_UPDATE, ON_UPDATE), \
        ::Engine::detail::auto_reg(#ON_EXIT, ON_EXIT)  \
    });

// ============================================================================
// Auto-registration macro — creates a static object whose constructor
// registers the blueprint and all referenced callbacks at program startup.
//
// Usage (typically at the bottom of a *StateCallbacks.h file):
//
//   REGISTER_STATE_MACHINE_BLUEPRINT(Player, "Idle",
//       STATE("Idle",    PlayerCB::idle_enter,    PlayerCB::idle_update,    NO_CALLBACK)
//       STATE("Walking", PlayerCB::walk_enter,    PlayerCB::walk_update,    PlayerCB::walk_exit)
//   )
// ============================================================================

#define REGISTER_STATE_MACHINE_BLUEPRINT(ID, INITIAL_STATE, ...) \
    namespace { \
      struct BlueprintRegistrar_##ID { \
        BlueprintRegistrar_##ID() { \
          ::Engine::StateMachineBlueprint bp; \
          bp.initial_state = INITIAL_STATE; \
          __VA_ARGS__ \
          ::Engine::StateMachineBlueprintRegistry::instance() \
            .register_blueprint(#ID, bp); \
        } \
      }; \
      static BlueprintRegistrar_##ID blueprint_registrar_##ID##_; \
    }

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

    class StateMachineBlueprintRegistry {
    public:
        static StateMachineBlueprintRegistry& instance() {
            static StateMachineBlueprintRegistry registry;
            return registry;
        }

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
// Auto-registration macro (same pattern as REGISTER_FUNCTION)
// Usage:
//   REGISTER_STATE_MACHINE_BLUEPRINT(TurretMonkey, "Idle",
//     { "Idle",    "Turret_monkey_idle_enter",    "Turret_monkey_idle_update",    ""                           },
//     { "Chasing", "Turret_monkey_chasing_enter",  "Turret_monkey_chasing_update", "Turret_monkey_chasing_exit" })
//
// NOTE: The state entries are passed as variadic args (...) because the
//       C preprocessor only respects () for argument grouping, not {}.
//       Commas inside { } would otherwise be treated as separate macro args.
// ============================================================================

#define REGISTER_STATE_MACHINE_BLUEPRINT(ID, INITIAL_STATE, ...) \
    namespace { \
      struct BlueprintRegistrar_##ID { \
        BlueprintRegistrar_##ID() { \
          Engine::StateMachineBlueprintRegistry::instance().register_blueprint( \
            #ID, Engine::StateMachineBlueprint({__VA_ARGS__}, INITIAL_STATE)); \
        } \
      }; \
      static BlueprintRegistrar_##ID blueprint_registrar_##ID##_; \
    }

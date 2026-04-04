//-----------------------------------------------------------------------------------
// file:    LuaManager.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the LuaManager singleton.
//
// Copyright (c) 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "Precompiled.h"
#include "LuaManager.h"

#include <sol/sol.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "LuaBindingRegistry.h"
#include "LuaBindings.h"
#include "EventManager/EventManager.h"
#include "Events/EngineEvents.h"
#include "ECS/ECS.h"
#include "ECS/Entity.h"
#include "ECS/Reflection.h"
#include "ECS/Prefab.h"
#include "FunctionRegistry.h"
#include "Logger.h"
#include "InputManager.h"
#include "KeyCodes.h"
#include "Time.h"
#include "Core.h"
#include "Audio/AudioSystem.h"
#include "SceneManager.h"
#include "LevelManager.h"
#include "WindowManager.h"
#include "FileIO/Settings.h"
#include "FileIO/SettingsManager.h"
#include "Math/Vector2D.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Path.h"
#include "Components/StateMachineBlueprintRegistry.h"
#include "Components/StateMachineComponents.h"
#include "Components/GraphicsComponents.h"
#include "Components/CollisionComponents.h"
#include "Components/PhysicsComponents.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/GraphicsSystem.h"
#include "Resources.h"
#include "UpgradeManager.h"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace Engine {

  // ============================================================================
  // Impl — defined here so that sol:: types don't leak into LuaManager.h
  // ============================================================================

  struct LuaManager::Impl {
    using Getter = std::function<sol::object(sol::state_view, ECS::Entity)>;
    std::unordered_map<std::string, Getter> component_getters;
  };

  // ============================================================================
  // Singleton
  // ============================================================================

  LuaManager& LuaManager::instance() {
    static LuaManager* inst = new LuaManager();
    return *inst;
  }

  void LuaManager::destroy_instance() {
    // Intentional leak pattern — matches FunctionRegistry, StateMachineBlueprintRegistry.
  }

  LuaManager::LuaManager() = default;
  LuaManager::~LuaManager() { shutdown(); }

  sol::state& LuaManager::lua() {
    return *lua_;
  }

  // ============================================================================
  // init
  // ============================================================================

  void LuaManager::init() {
    if (lua_) {
      Log::warning("[LuaManager] Already initialised — skipping.");
      return;
    }

    lua_ = new sol::state();
    impl_ = new Impl();

    lua_->open_libraries(
      sol::lib::base,
      sol::lib::math,
      sol::lib::string,
      sol::lib::table,
      sol::lib::os,
      sol::lib::coroutine
    );

    Log::info("[LuaManager] sol::state created, draining binding registry...");

    // 1. Pre-register Vec2D with full constructor/operators/methods
    //    (must happen BEFORE drain so the generic binder skips Vec2D)
    bind_vec2d_extras();

    // 2. Drain the side registry — binds every reflected component type
    //    (also drains method binder entries in a second pass)
    drain_binding_registry();

    // 3. Bind Entity with __index metamethod for component access
    //    (includes has/remove/add by short name)
    bind_entity_type();

    // 4. Bind engine-side component methods (StateMachine, Animation, AABB)
    bind_statemachine_methods();
    bind_animation_methods();
    bind_aabb_methods();

    // 5. Bind global ECS table (includes get_first_entity_with, instantiate_prefab)
    bind_ecs_table();

    // 6. Bind Physics & Systems
    bind_physics();

    // 7. Bind Resources (Prefab, load_prefab, load_texture)
    bind_resources();

    // 8. Bind engine subsystem APIs
    bind_log();
    bind_input();
    bind_time();
    bind_audio();
    bind_settings();
    bind_app();
    bind_scene_manager();
    bind_level_manager();

    // 9. Bind register_blueprint() so scripts can call it during load
    bind_register_blueprint();

    // 10. Load user scripts (register_blueprint() calls happen during execution)
    load_scripts();

    // 11. Start background file watcher for hot-reload
    start_file_watcher();

    Log::info("[LuaManager] Initialisation complete. {} component types bound.",
              impl_->component_getters.size());
  }

  // ============================================================================
  // shutdown
  // ============================================================================

  void LuaManager::shutdown() {
    stop_file_watcher();

    // Remove Lua-registered functions from FunctionRegistry BEFORE deleting
    // the lua_State.  These FunctionRegistry entries hold std::function lambdas
    // that captured sol::protected_function by value.  Their destructors call
    // luaL_unref, which requires the lua_State to still be alive.
    // UnregisterGameLogic() also calls FunctionRegistry::clear_all(), but that
    // runs AFTER this shutdown — by which point the lua_State is gone.
    {
      auto& fr = FunctionRegistry::instance();
      using Sig0 = void();
      using Sig1 = void(ECS::Entity);
      using Sig2 = void(ECS::Entity, ECS::Entity);
      for (const auto& name : lua_registered_functions_) {
        fr.unregister_function<Sig0>(name);
        fr.unregister_function<Sig1>(name);
        fr.unregister_function<Sig2>(name);
      }
      lua_registered_functions_.clear();
    }

    if (lua_) {
      delete lua_;
      lua_ = nullptr;
    }
    if (impl_) {
      delete impl_;
      impl_ = nullptr;
    }
  }

  // ============================================================================
  // drain_binding_registry
  // ============================================================================

  void LuaManager::drain_binding_registry() {
    auto& reg = LuaBindingRegistry::instance();

    // Pass 1: bind component usertypes and store getters.
    // IMPORTANT: The REGISTER_COMPONENT macro lives in a header included by
    // every TU in both Engine and GameLogic DLLs.  Each TU pushes its own
    // entry, so the registry contains many duplicates per component type.
    // Sol2 keys its internal metatables on the *address* of a static template
    // variable (unique_usertype_id<T>::value), which differs across DLLs.
    // If the binder comes from DLL A but the getter from DLL B, the getter's
    // sol::make_object can't find the metatable registered by the binder.
    // Fix: only process the FIRST entry for each component name.  Engine DLL
    // entries are pushed before GameLogic entries (dependency load order), so
    // both binder and getter come from the same DLL.
    for (auto& entry : reg.entries()) {
      if (impl_->component_getters.count(entry.short_name)) continue; // skip duplicates

      // Bind the component's usertype (recursively binds field types too)
      entry.binder(*lua_);

      // Store the component getter for the Entity __index metamethod
      impl_->component_getters[entry.short_name] = entry.getter;

      Log::info("[LuaManager] Bound component '{}'", entry.short_name);
    }

    // Pass 2: augment usertypes with extra methods (from LUA_BIND_METHODS)
    // Method binders access lua["TypeName"]["method"], which goes through
    // the Lua global table (not Sol2's internal type ID), so cross-DLL
    // duplicates are harmless — but we deduplicate for efficiency.
    std::unordered_set<std::string> bound_methods;
    for (auto& entry : reg.method_binder_entries()) {
      if (bound_methods.count(entry.short_name)) continue;
      bound_methods.insert(entry.short_name);

      entry.binder(*lua_);
      Log::info("[LuaManager] Bound methods for '{}'", entry.short_name);
    }
  }

  // ============================================================================
  // Phase 1: bind_vec2d_extras — full Vec2D (constructor, operators, methods)
  // Must run BEFORE drain_binding_registry() so the generic binder skips Vec2D.
  // ============================================================================

  void LuaManager::bind_vec2d_extras() {
    auto& L = *lua_;

    // Mark as bound so drain_binding_registry() won't re-register Vec2D
    LuaBindings::mark_bound<Math::Vec2D>();

    L.new_usertype<Math::Vec2D>("Vec2D",
      // Constructors
      sol::constructors<Math::Vec2D(), Math::Vec2D(f32, f32)>(),

      // Fields
      "x", &Math::Vec2D::x,
      "y", &Math::Vec2D::y,

      // Methods
      "zero",             &Math::Vec2D::zero,
      "get_magnitude",    &Math::Vec2D::get_magnitude,
      "get_magnitude_sq", &Math::Vec2D::get_magnitude_sq,

      // Operators
      sol::meta_function::addition, sol::resolve<Math::Vec2D(Math::Vec2D const&, Math::Vec2D const&)>(Math::operator+),
      sol::meta_function::subtraction, sol::resolve<Math::Vec2D(Math::Vec2D const&, Math::Vec2D const&)>(Math::operator-),
      sol::meta_function::multiplication, [](const Math::Vec2D& v, f32 s) -> Math::Vec2D { return v * s; },
      sol::meta_function::division, sol::resolve<Math::Vec2D(Math::Vec2D const&, f32)>(Math::operator/),
      sol::meta_function::unary_minus, [](const Math::Vec2D& v) -> Math::Vec2D { return Math::Vec2D(-v.x, -v.y); },
      sol::meta_function::equal_to, [](const Math::Vec2D& a, const Math::Vec2D& b) -> bool {
        return a.x == b.x && a.y == b.y;
      },
      sol::meta_function::to_string, [](const Math::Vec2D& v) -> std::string {
        return "Vec2D(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
      }
    );

    // Free math functions (disambiguated — Vec3D/Vec4D overloads exist)
    L["normalise_vector"] = sol::resolve<Math::Vec2D(Math::Vec2D const&)>(Math::normalise_vector);
    L["dot_product"]      = sol::resolve<f32(Math::Vec2D const&, Math::Vec2D const&)>(Math::dot_product);
    L["cross_product"]    = sol::resolve<f32(Math::Vec2D const&, Math::Vec2D const&)>(Math::cross_product);
    L["rotate"]           = sol::resolve<Math::Vec2D(Math::Vec2D const&, f32)>(Math::rotate);
    L["scale_vector"]     = sol::resolve<Math::Vec2D(Math::Vec2D const&, f32)>(Math::scale_vector);
  }

  // ============================================================================
  // Phase 3: bind_statemachine_methods
  // ============================================================================

  void LuaManager::bind_statemachine_methods() {
    auto& L = *lua_;
    L["StateMachine"]["change_state"]              = &Components::StateMachine::change_state;
    L["StateMachine"]["is_in_state"]               = &Components::StateMachine::is_in_state;
    L["StateMachine"]["get_current_state"]          = &Components::StateMachine::get_current_state;
    L["StateMachine"]["has_state"]                  = &Components::StateMachine::has_state;
    L["StateMachine"]["is_first_frame"]             = &Components::StateMachine::is_first_frame;
    L["StateMachine"]["revert_to_previous_state"]   = &Components::StateMachine::revert_to_previous_state;
    L["StateMachine"]["is_property_anim_playing"]   = &Components::StateMachine::is_property_anim_playing;
  }

  // ============================================================================
  // Phase 3: bind_animation_methods
  // ============================================================================

  void LuaManager::bind_animation_methods() {
    auto& L = *lua_;
    L["Animation"]["has_ended"]               = &Components::Animation::has_ended;
    L["Animation"]["get_current_frame_index"]  = &Components::Animation::get_current_frame_index;
    L["Animation"]["get_elapsed_time"]         = &Components::Animation::get_elapsed_time;
  }

  // ============================================================================
  // Phase 3: bind_aabb_methods
  // ============================================================================

  void LuaManager::bind_aabb_methods() {
    auto& L = *lua_;
    L["AABB"]["intersects"] = &::AABB::intersects;
  }

  // ============================================================================
  // Phase 5: bind_physics — Physics.add_force, Systems.add_camera_shake
  // ============================================================================

  void LuaManager::bind_physics() {
    auto& L = *lua_;

    sol::table physics = L.create_named_table("Physics");
    physics["add_force"] = [](ECS::Entity entity, f32 fx, f32 fy) {
      auto* rb = entity.get_component<Components::RigidBody>();
      if (rb) {
        Systems::Physics2DSystem::add_force(*rb, Math::Vec2D(fx, fy));
      }
    };

    sol::table systems = L.create_named_table("Systems");
    systems["add_camera_shake"] = [](f32 trauma) {
      Systems::add_camera_shake(trauma);
    };
  }

  // ============================================================================
  // Phase 6: bind_resources — Prefab handle, load_prefab, load_texture
  // ============================================================================

  void LuaManager::bind_resources() {
    auto& L = *lua_;

    // Register Prefab as an opaque handle
    L.new_usertype<ECS::Prefab>("Prefab", sol::no_constructor);

    sol::table res = L.create_named_table("Resources");
    res["load_prefab"] = [](const std::string& path) -> ECS::Prefab {
      return Resources::load_prefab(path);
    };
    res["load_texture"] = [](const std::string& path, sol::optional<bool> persist) {
      return Resources::load_texture(path, true, persist.value_or(false));
    };

    // Upgrade table (Engine-side singleton)
    sol::table upgrade = L.create_named_table("Upgrade");
    upgrade["reset"] = []() {
      UpgradeManager::instance().reset();
    };
    upgrade["get_points"] = []() -> int {
      return UpgradeManager::instance().get_points();
    };
    upgrade["add_points"] = [](int amt) {
      UpgradeManager::instance().add_points(amt);
    };
  }

  // ============================================================================
  // bind_entity_type
  // ============================================================================

  void LuaManager::bind_entity_type() {
    auto& L = *lua_;

    auto entity_type = L.new_usertype<ECS::Entity>("Entity",
      sol::constructors<ECS::Entity(), ECS::Entity(ECS::EntityID)>(),

      // Identity
      "get_id",    &ECS::Entity::get_id,
      "is_valid",  &ECS::Entity::is_valid,
      "destroy",   &ECS::Entity::destroy,

      // Naming
      "set_name",  &ECS::Entity::set_name,
      "get_name",  &ECS::Entity::get_name,

      // Layer management (string-based)
      "add_to_layer",      &ECS::Entity::add_to_layer,
      "remove_from_layer", &ECS::Entity::remove_from_layer,
      "clear_layers",      &ECS::Entity::clear_layers,

      // add_component_by_name
      "add_component_by_name", &ECS::Entity::add_component_by_name
    );

    // Overloaded is_on_layer — pick the string version
    entity_type["is_on_layer"] = static_cast<bool(ECS::Entity::*)(const std::string&) const>(
      &ECS::Entity::is_on_layer
    );

    // ── Phase 2: has / remove / add by short-name string ──────────────
    // Build a resolution map: short name -> { full name, ComponentTypeID }
    // This lets Lua call entity:has("Attack"), entity:remove("Parent"), entity:add("Lifetime")

    entity_type["has"] = [](ECS::Entity entity, const std::string& short_name) -> bool {
      auto& type_map = ECS::Serialization::get_component_type_map();
      auto& containers = ECS::Internal::get_containers();
      ECS::EntityID eid = entity.get_id();
      // Search for matching component type (short name == last part after ::)
      for (auto& [full_name, type_id] : type_map) {
        auto pos = full_name.rfind(':');
        std::string_view sn = (pos != std::string::npos) ? std::string_view(full_name).substr(pos + 1) : full_name;
        if (sn == short_name) {
          return eid < containers.entity_signatures.size() && containers.entity_signatures[eid].test(type_id);
        }
      }
      return false;
    };

    entity_type["add"] = [](ECS::Entity entity, const std::string& short_name) -> bool {
      auto& type_map = ECS::Serialization::get_component_type_map();
      for (auto& [full_name, type_id] : type_map) {
        auto pos = full_name.rfind(':');
        std::string_view sn = (pos != std::string::npos) ? std::string_view(full_name).substr(pos + 1) : full_name;
        if (sn == short_name) {
          return entity.add_component_by_name(full_name);
        }
      }
      Log::warning("[Lua] entity:add('{}') — unknown component", short_name);
      return false;
    };

    entity_type["remove"] = [](ECS::Entity entity, const std::string& short_name) {
      auto& type_map = ECS::Serialization::get_component_type_map();
      auto& containers = ECS::Internal::get_containers();
      ECS::EntityID eid = entity.get_id();
      for (auto& [full_name, type_id] : type_map) {
        auto pos = full_name.rfind(':');
        std::string_view sn = (pos != std::string::npos) ? std::string_view(full_name).substr(pos + 1) : full_name;
        if (sn == short_name) {
          // Queue for removal (same as template remove_component)
          containers.components_to_remove.emplace_back(eid, type_id);
          return;
        }
      }
      Log::warning("[Lua] entity:remove('{}') — unknown component", short_name);
    };

    // ── __index metamethod for dot-syntax component access ────────────
    // When Lua does `entity.Transform2D`, Sol2 first checks the usertype
    // metatable. If no match, it calls __index. We look up the key in
    // the component getter map and return a pointer to the live C++ component.
    Impl* pimpl = impl_;
    entity_type[sol::meta_function::index] = [pimpl](sol::this_state ts, ECS::Entity& entity, const std::string& key) -> sol::object {
      auto it = pimpl->component_getters.find(key);
      if (it != pimpl->component_getters.end()) {
        return it->second(sol::state_view(ts), entity);
      }
      return sol::lua_nil;
    };
  }

  // ============================================================================
  // bind_ecs_table
  // ============================================================================

  void LuaManager::bind_ecs_table() {
    auto& L = *lua_;

    sol::table ecs = L.create_named_table("ECS");

    ecs["create_entity"] = []() -> ECS::Entity {
      return ECS::create_entity();
    };

    ecs["get_entity"] = [](ECS::EntityID id) -> ECS::Entity {
      return ECS::get_entity(id);
    };

    ecs["destroy_entity"] = [](ECS::Entity e) {
      ECS::destroy_entity(e);
    };

    ecs["get_all_entities"] = []() -> std::vector<ECS::Entity> {
      return ECS::get_all_entities();
    };

    // get_entities_on_layer — pick the string overload
    ecs["get_entities_on_layer"] = [](const std::string& layer_name) -> std::vector<ECS::Entity> {
      return ECS::get_entities_on_layer(layer_name);
    };

    ecs["get_entity_count"] = []() -> size_t {
      return ECS::get_entity_count();
    };

    ecs["clone_entity"] = [](ECS::Entity src) -> ECS::Entity {
      return ECS::clone_entity(src);
    };

    // Phase 6: instantiate_prefab
    ecs["instantiate_prefab"] = [](const ECS::Prefab& prefab) -> ECS::Entity {
      return ECS::instantiate_prefab(prefab);
    };

    // Phase 7: get_first_entity_with — find first active entity that has a component by short name
    ecs["get_first_entity_with"] = [](sol::this_state ts, const std::string& short_name) -> sol::object {
      auto& type_map = ECS::Serialization::get_component_type_map();
      auto& containers = ECS::Internal::get_containers();

      // Resolve short name to ComponentTypeID
      ECS::ComponentTypeID target_id = ECS::MAX_COMPONENTS;
      for (auto& [full_name, type_id] : type_map) {
        auto pos = full_name.rfind(':');
        std::string_view sn = (pos != std::string::npos) ? std::string_view(full_name).substr(pos + 1) : full_name;
        if (sn == short_name) {
          target_id = type_id;
          break;
        }
      }

      if (target_id == ECS::MAX_COMPONENTS) {
        Log::warning("[Lua] ECS.get_first_entity_with('{}') — unknown component", short_name);
        return sol::lua_nil;
      }

      // Iterate active entities and return the first one with this component
      for (auto eid : containers.active_entities) {
        if (eid < containers.entity_signatures.size() && containers.entity_signatures[eid].test(target_id)) {
          return sol::make_object(sol::state_view(ts), ECS::Entity(eid));
        }
      }
      return sol::lua_nil;
    };

    ecs["get_entity_by_name"] = [](sol::this_state ts, const std::string& name) -> sol::object {
      for (auto entity : ECS::get_all_entities()) {
        if (entity.get_name() == name) {
          return sol::make_object(sol::state_view(ts), entity);
        }
      }
      return sol::lua_nil;
    };
  }

  // ============================================================================
  // bind_log
  // ============================================================================

  void LuaManager::bind_log() {
    auto& L = *lua_;

    sol::table log_table = L.create_named_table("Log");

    log_table["info"]    = [](const std::string& msg) { Log::detail::info_impl(msg);    };
    log_table["warning"] = [](const std::string& msg) { Log::detail::warning_impl(msg); };
    log_table["error"]   = [](const std::string& msg) { Log::detail::error_impl(msg);   };
  }

  // ============================================================================
  // bind_input
  // ============================================================================

  void LuaManager::bind_input() {
    auto& L = *lua_;

    sol::table input = L.create_named_table("Input");

    input["is_pressed"]  = [](Engine::KeyCode k) { return Engine::Input::is_pressed(k);  };
    input["is_held"]     = [](Engine::KeyCode k) { return Engine::Input::is_held(k);     };
    input["is_released"] = [](Engine::KeyCode k) { return Engine::Input::is_released(k); };
    input["was_last_input_mouse"] = []() { return Engine::Input::was_last_input_mouse(); };

    // Mouse position — returns x, y as two values
    input["get_mouse_position"] = [](sol::this_state ts) -> sol::object {
      auto pos = Engine::Input::get_mouse_position();
      sol::state_view lua(ts);
      sol::table t = lua.create_table();
      t["x"] = pos.x;
      t["y"] = pos.y;
      return t;
    };

    input["get_mouse_position_if_in_viewport"] = [](sol::this_state ts) -> sol::object {
      auto pos = Engine::Input::get_mouse_position_if_in_viewport();
      sol::state_view lua(ts);
      if (!pos.has_value()) {
        return sol::make_object(lua, sol::nil);
      }

      sol::table t = lua.create_table();
      t["x"] = pos->x;
      t["y"] = pos->y;
      return t;
    };

    input["get_mouse_world_position"] = [](sol::this_state ts) -> sol::object {
      auto pos = Engine::Input::get_mouse_world_position();
      sol::state_view lua(ts);
      sol::table t = lua.create_table();
      t["x"] = pos.x;
      t["y"] = pos.y;
      return t;
    };

    input["get_scroll_offset"] = [](sol::this_state ts) -> sol::object {
      auto off = Engine::Input::get_scroll_offset();
      sol::state_view lua(ts);
      sol::table t = lua.create_table();
      t["x"] = off.x;
      t["y"] = off.y;
      return t;
    };

    // ── KeyCode enum table ────────────────────────────────────────────
    sol::table key = L.create_named_table("Key");

    // Mouse
    key["MouseLeft"]    = Engine::KeyCode::MouseLeft;
    key["MouseRight"]   = Engine::KeyCode::MouseRight;
    key["MouseMiddle"]  = Engine::KeyCode::MouseMiddle;

    // Letters
    key["A"] = Engine::KeyCode::A;  key["B"] = Engine::KeyCode::B;
    key["C"] = Engine::KeyCode::C;  key["D"] = Engine::KeyCode::D;
    key["E"] = Engine::KeyCode::E;  key["F"] = Engine::KeyCode::F;
    key["G"] = Engine::KeyCode::G;  key["H"] = Engine::KeyCode::H;
    key["I"] = Engine::KeyCode::I;  key["J"] = Engine::KeyCode::J;
    key["K"] = Engine::KeyCode::K;  key["L"] = Engine::KeyCode::L;
    key["M"] = Engine::KeyCode::M;  key["N"] = Engine::KeyCode::N;
    key["O"] = Engine::KeyCode::O;  key["P"] = Engine::KeyCode::P;
    key["Q"] = Engine::KeyCode::Q;  key["R"] = Engine::KeyCode::R;
    key["S"] = Engine::KeyCode::S;  key["T"] = Engine::KeyCode::T;
    key["U"] = Engine::KeyCode::U;  key["V"] = Engine::KeyCode::V;
    key["W"] = Engine::KeyCode::W;  key["X"] = Engine::KeyCode::X;
    key["Y"] = Engine::KeyCode::Y;  key["Z"] = Engine::KeyCode::Z;

    // Numbers
    key["Num0"] = Engine::KeyCode::Num0;  key["Num1"] = Engine::KeyCode::Num1;
    key["Num2"] = Engine::KeyCode::Num2;  key["Num3"] = Engine::KeyCode::Num3;
    key["Num4"] = Engine::KeyCode::Num4;  key["Num5"] = Engine::KeyCode::Num5;
    key["Num6"] = Engine::KeyCode::Num6;  key["Num7"] = Engine::KeyCode::Num7;
    key["Num8"] = Engine::KeyCode::Num8;  key["Num9"] = Engine::KeyCode::Num9;

    // Special keys
    key["Space"]      = Engine::KeyCode::Space;
    key["Enter"]      = Engine::KeyCode::Enter;
    key["Escape"]     = Engine::KeyCode::Escape;
    key["Tab"]        = Engine::KeyCode::Tab;
    key["Backspace"]  = Engine::KeyCode::Backspace;
    key["ShiftLeft"]  = Engine::KeyCode::ShiftLeft;
    key["ShiftRight"] = Engine::KeyCode::ShiftRight;
    key["CtrlLeft"]   = Engine::KeyCode::CtrlLeft;
    key["CtrlRight"]  = Engine::KeyCode::CtrlRight;
    key["AltLeft"]    = Engine::KeyCode::AltLeft;
    key["AltRight"]   = Engine::KeyCode::AltRight;
    key["Delete"]     = Engine::KeyCode::Delete;

    // Arrows
    key["Up"]    = Engine::KeyCode::Up;
    key["Down"]  = Engine::KeyCode::Down;
    key["Left"]  = Engine::KeyCode::Left;
    key["Right"] = Engine::KeyCode::Right;

    // Function keys
    key["F1"]  = Engine::KeyCode::F1;   key["F2"]  = Engine::KeyCode::F2;
    key["F3"]  = Engine::KeyCode::F3;   key["F4"]  = Engine::KeyCode::F4;
    key["F5"]  = Engine::KeyCode::F5;   key["F6"]  = Engine::KeyCode::F6;
    key["F7"]  = Engine::KeyCode::F7;   key["F8"]  = Engine::KeyCode::F8;
    key["F9"]  = Engine::KeyCode::F9;   key["F10"] = Engine::KeyCode::F10;
    key["F11"] = Engine::KeyCode::F11;  key["F12"] = Engine::KeyCode::F12;

    // Brackets
    key["LeftBracket"]  = Engine::KeyCode::LeftBracket;
    key["RightBracket"] = Engine::KeyCode::RightBracket;
  }

  // ============================================================================
  // bind_time
  // ============================================================================

  void LuaManager::bind_time() {
    auto& L = *lua_;

    sol::table time = L.create_named_table("Time");

    time["dt"]              = []() { return Engine::Time::get_fixed_dt(); };
    time["get_fixed_dt"]    = []() { return Engine::Time::get_fixed_dt(); };
    time["get_unscaled_dt"] = []() { return Engine::Time::get_unscaled_fixed_dt(); };
    time["get_real_dt"]     = []() { return Engine::Time::get_real_dt(); };
    time["get_time"]        = []() { return Engine::Time::get_time(); };
    time["get_unscaled_time"] = []() { return Engine::Time::get_unscaled_time(); };
    time["get_time_scale"]  = []() { return Engine::Time::get_time_scale(); };
    time["set_time_scale"]  = [](float s) { Engine::Time::set_time_scale(s); };
    time["get_paused"]      = []() { return Engine::Time::get_paused(); };
    time["set_paused"]      = [](bool p) { return Engine::Time::set_paused(p); };
    time["toggle_paused"]   = []() { return Engine::Time::toggle_paused(); };
  }

  // ============================================================================
  // bind_audio
  // ============================================================================

  void LuaManager::bind_audio() {
    auto& L = *lua_;

    sol::table audio = L.create_named_table("Audio");

    // One-shot playback
    audio["play_event"] = [](const std::string& path) {
      return Engine::AudioSystem::playEvent(path);
    };

    audio["stop_event"] = [](const std::string& path, sol::optional<bool> immediate) {
      return Engine::AudioSystem::stopEvent(path, immediate.value_or(true));
    };

    audio["play_event_at_position"] = [](const std::string& path, float x, float y) {
      return Engine::AudioSystem::playEventAtPosition(path, Math::Vec2D{x, y});
    };

    // Instance management
    audio["create_instance"] = [](const std::string& name, const std::string& path) {
      return Engine::AudioSystem::createEventInstance(name, path);
    };

    audio["start_instance"] = [](const std::string& name) {
      return Engine::AudioSystem::startEventInstance(name);
    };

    audio["stop_instance"] = [](const std::string& name, sol::optional<bool> immediate) {
      return Engine::AudioSystem::stopEventInstance(name, immediate.value_or(false));
    };

    audio["release_instance"] = [](const std::string& name) {
      return Engine::AudioSystem::releaseEventInstance(name);
    };

    audio["is_instance_playing"] = [](const std::string& name) {
      return Engine::AudioSystem::isEventInstancePlaying(name);
    };

    // Parameters
    audio["set_parameter"] = [](const std::string& name, const std::string& param, float val) {
      return Engine::AudioSystem::setEventParameter(name, param, val);
    };

    // Volume
    audio["set_master_volume"] = [](float v) { Engine::AudioSystem::setMasterVolume(v); };
    audio["get_master_volume"] = []() { return Engine::AudioSystem::getMasterVolume(); };
    audio["set_bgm_volume"]   = [](float v) { Engine::AudioSystem::setBGMVolume(v); };
    audio["get_bgm_volume"]   = []() { return Engine::AudioSystem::getBGMVolume(); };
    audio["set_sfx_volume"]   = [](float v) { Engine::AudioSystem::setSFXVolume(v); };
    audio["get_sfx_volume"]   = []() { return Engine::AudioSystem::getSFXVolume(); };

    // Scene music
    audio["play_scene_music"] = [](const std::string& path, sol::optional<float> vol) {
      return Engine::AudioSystem::playSceneMusic(path, vol.value_or(1.0f));
    };
    audio["stop_scene_music"] = [](sol::optional<bool> immediate) {
      return Engine::AudioSystem::stopSceneMusic(immediate.value_or(true));
    };
    audio["is_scene_music_playing"] = []() {
      return Engine::AudioSystem::isSceneMusicPlaying();
    };

    // Global
    audio["pause_all"] = [](bool p) { Engine::AudioSystem::pauseAll(p); };
    audio["stop_all"]  = [](sol::optional<bool> immediate) {
      Engine::AudioSystem::stopAll(immediate.value_or(false));
    };
  }

  // ============================================================================
  // bind_settings
  // ============================================================================

  void LuaManager::bind_settings() {
    auto& L = *lua_;

    sol::table settings = L.create_named_table("Settings");

    settings["get_fullscreen"] = []() {
      return Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get().fullscreen;
    };

    settings["set_fullscreen"] = [](bool fullscreen) {
      auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
      game_settings.fullscreen = fullscreen;
      Engine::WindowManager::set_fullscreen(game_settings.fullscreen);
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
      return game_settings.fullscreen;
    };

    settings["get_vsync"] = []() {
      return Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get().vsync;
    };

    settings["set_vsync"] = [](bool enabled) {
      auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
      game_settings.vsync = enabled;
      Engine::WindowManager::set_vsync(game_settings.vsync);
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
      return game_settings.vsync;
    };

    settings["get_bgm_volume"] = []() {
      return Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get().bgm_volume;
    };

    settings["set_bgm_volume"] = [](float volume) {
      auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
      game_settings.bgm_volume = std::clamp(volume, 0.0f, 1.0f);
      Engine::AudioSystem::setBGMVolume(game_settings.bgm_volume);
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
      return game_settings.bgm_volume;
    };

    settings["get_sfx_volume"] = []() {
      return Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get().sfx_volume;
    };

    settings["set_sfx_volume"] = [](float volume) {
      auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
      game_settings.sfx_volume = std::clamp(volume, 0.0f, 1.0f);
      Engine::AudioSystem::setSFXVolume(game_settings.sfx_volume);
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
      return game_settings.sfx_volume;
    };

    settings["save_game"] = []() {
      return Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
    };

    settings["reload_game"] = []() {
      if (!Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::load("Assets/settings/game_settings.json")) {
        return false;
      }

      const auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get();
      Engine::WindowManager::set_fullscreen(game_settings.fullscreen);
      Engine::WindowManager::set_vsync(game_settings.vsync);
      Engine::AudioSystem::setBGMVolume(game_settings.bgm_volume);
      Engine::AudioSystem::setSFXVolume(game_settings.sfx_volume);
      return true;
    };
  }

  // ============================================================================
  // bind_app
  // ============================================================================

  void LuaManager::bind_app() {
    auto& L = *lua_;

    sol::table app = L.create_named_table("App");
    app["stop"] = []() {
      Engine::Core::stop();
    };
  }

  // ============================================================================
  // bind_scene_manager
  // ============================================================================

  void LuaManager::bind_scene_manager() {
    auto& L = *lua_;

    sol::table scene = L.create_named_table("Scene");

    scene["load"]      = [](const std::string& name) { Engine::SceneManager::load_scene(name); };
    scene["push"]      = [](const std::string& name) { Engine::SceneManager::push_scene(name); };
    scene["pop"]       = []()                         { Engine::SceneManager::pop_scene();       };
    scene["current"]   = []() -> std::string          { return Engine::SceneManager::get_current_scene_name(); };
    scene["get_all"]   = []() -> std::vector<std::string> { return Engine::SceneManager::get_all_scene_names(); };
  }

  // ============================================================================
  // bind_level_manager
  // ============================================================================

  void LuaManager::bind_level_manager() {
    auto& L = *lua_;

    sol::table level = L.create_named_table("Level");

    level["open"]          = [](const std::string& path) { return Engine::LevelManager::open_level(path); };
    level["switch_to"]     = [](const std::string& id)   { Engine::LevelManager::switch_to_level(id); };
    level["reload"]        = [](const std::string& id)   { Engine::LevelManager::reload_level(id); };
    level["close"]         = [](const std::string& id)   { return Engine::LevelManager::close_level(id); };
    level["get_current_path"] = []() -> std::string       { return Engine::LevelManager::get_current_level_path(); };
    level["get_active_id"]    = []() -> std::string       { return Engine::LevelManager::get_active_level_id(); };
    level["is_open"]       = [](const std::string& id)   { return Engine::LevelManager::is_level_open(id); };
  }

  // ============================================================================
  // bind_register_blueprint — expose register_blueprint(name, table) to Lua
  // Scripts call this inline to register state machine blueprints.
  // ============================================================================

  void LuaManager::bind_register_blueprint() {
    auto& L = *lua_;

    L["register_blueprint"] = [this](const std::string& name, sol::table table) {
      auto& bp_registry = StateMachineBlueprintRegistry::instance();

      StateMachineBlueprint blueprint;

      sol::optional<std::string> initial = table["initial_state"];
      if (initial) {
        blueprint.initial_state = *initial;
      }

      sol::optional<sol::table> states_table = table["states"];
      if (states_table) {
        states_table->for_each([&](sol::object /*idx*/, sol::object val) {
          if (val.get_type() != sol::type::table) return;
          sol::table state_entry = val.as<sol::table>();

          StateBlueprint sb;
          sb.state_name = state_entry.get_or<std::string>("name", "");
          sb.on_enter   = state_entry.get_or<std::string>("on_enter", "");
          sb.on_update  = state_entry.get_or<std::string>("on_update", "");
          sb.on_exit    = state_entry.get_or<std::string>("on_exit", "");

          if (!sb.state_name.empty()) {
            blueprint.states.push_back(std::move(sb));
          }
        });
      }

      if (blueprint.states.empty()) {
        Log::warning("[LuaManager] register_blueprint('{}') — no states, skipping.", name);
        return;
      }

      bp_registry.register_lua_blueprint(name, blueprint);

      // Record source file for editor lookup
      if (!current_script_path_.empty()) {
        blueprint_source_files_[name] = current_script_path_;
      }

      Log::info("[LuaManager] Registered Lua blueprint '{}' with {} states.",
                name, blueprint.states.size());
    };
  }

  // ============================================================================
  // to_snake_case / to_lower — string helpers for Lua file generation
  // ============================================================================

  std::string LuaManager::to_snake_case(const std::string& name) {
    std::string result;
    for (size_t i = 0; i < name.size(); ++i) {
      char c = name[i];
      if (std::isupper(c)) {
        if (i > 0) result += '_';
        result += static_cast<char>(std::tolower(c));
      } else {
        result += c;
      }
    }
    return result;
  }

  std::string LuaManager::to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
  }

  // ============================================================================
  // load_scripts — execute every .lua file in Assets/scripts/
  // ============================================================================

  void LuaManager::load_scripts() {
    auto& L = *lua_;

    // Resolve the scripts directory relative to the working directory.
    std::filesystem::path scripts_dir = "Assets/scripts";
    if (!std::filesystem::exists(scripts_dir)) {
      Log::info("[LuaManager] No scripts directory found at '{}'. Skipping.", scripts_dir.string());
      return;
    }

    // Collect all .lua files recursively (including blueprints subdirectory)
    // Blueprint files define functions too, so we load them here as regular scripts.
    std::vector<std::filesystem::path> lua_files;
    for (auto& entry : std::filesystem::recursive_directory_iterator(scripts_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".lua") {
        lua_files.push_back(entry.path());
      }
    }

    // Sort for deterministic load order
    std::sort(lua_files.begin(), lua_files.end());

    for (auto& file_path : lua_files) {
      std::string path_str = file_path.string();

      // Set current_script_path_ so register_blueprint() can record its source
      current_script_path_ = std::filesystem::absolute(file_path).string();

      auto result = L.safe_script_file(path_str, sol::script_pass_on_error);
      if (!result.valid()) {
        sol::error err = result;
        Log::error("[LuaManager] Error loading '{}': {}", path_str, err.what());
        continue;
      }

      Log::info("[LuaManager] Loaded script '{}'", file_path.filename().string());
    }

    // Clear current_script_path_ after loading all scripts
    current_script_path_.clear();

    // ── Auto-register top-level Lua functions into FunctionRegistry ──
    // After executing ALL scripts, scan the global table for functions.
    // We register them as std::function<void(ECS::Entity)> AND
    // std::function<void(ECS::Entity, ECS::Entity)> so they can be used
    // by both single-entity callbacks (StateMachine) and two-entity
    // callbacks (CheckpointCallbacks) via SerializableFunction.
    sol::table globals = L.globals();
    globals.for_each([&](sol::object key_obj, sol::object val_obj) {
      if (key_obj.get_type() != sol::type::string) return;
      if (val_obj.get_type() != sol::type::function) return;

      std::string func_name = key_obj.as<std::string>();

      // Skip Lua built-in globals
      static const std::unordered_set<std::string> builtins = {
        "print", "type", "tostring", "tonumber", "pairs", "ipairs",
        "next", "select", "error", "pcall", "xpcall", "assert",
        "require", "rawget", "rawset", "rawequal", "rawlen",
        "setmetatable", "getmetatable", "load", "dofile", "loadfile",
        "collectgarbage", "coroutine", "string", "table", "math", "os",
        "io", "debug", "package", "utf8", "warn"
      };
      if (builtins.count(func_name)) return;

      // Skip if already registered as a C++ function (not one we registered from Lua before)
      auto& fr = FunctionRegistry::instance();
      using Sig0 = void();
      using Sig1 = void(ECS::Entity);
      using Sig2 = void(ECS::Entity, ECS::Entity);
      if (lua_registered_functions_.count(func_name) == 0 &&
          (fr.has_function<Sig0>(func_name) ||
           fr.has_function<Sig1>(func_name) ||
           fr.has_function<Sig2>(func_name))) {
        return;
      }

      // Capture the Lua function and wrap it
      sol::protected_function lua_func = val_obj.as<sol::protected_function>();
      std::string captured_name = func_name; // capture for lambda

      // Register zero-argument signature for UI click callbacks.
      fr.register_function<Sig0>(func_name, std::function<Sig0>(
        [lua_func, captured_name]() {
          auto result = lua_func();
          if (!result.valid()) {
            sol::error err = result;
            Log::error("[Lua] Error calling '{}': {}", captured_name, err.what());
          }
        }
      ));

      // Register single-entity signature
      fr.register_function<Sig1>(func_name, std::function<Sig1>(
        [lua_func, captured_name](ECS::Entity entity) {
          auto result = lua_func(entity);
          if (!result.valid()) {
            sol::error err = result;
            Log::error("[Lua] Error calling '{}': {}", captured_name, err.what());
          }
        }
      ));

      // Register two-entity signature (for collision/checkpoint callbacks)
      fr.register_function<Sig2>(func_name, std::function<Sig2>(
        [lua_func, captured_name](ECS::Entity entity_a, ECS::Entity entity_b) {
          auto result = lua_func(entity_a, entity_b);
          if (!result.valid()) {
            sol::error err = result;
            Log::error("[Lua] Error calling '{}': {}", captured_name, err.what());
          }
        }
      ));

      lua_registered_functions_.insert(func_name);
      Log::info("[LuaManager] Registered Lua function '{}' into FunctionRegistry", func_name);
    });
  }

  // ============================================================================
  // restore_after_dll_reload — re-populate FunctionRegistry and blueprints
  // after an external clear_all() caused by GameLogic DLL hot-reload.
  // Does NOT call fr.unregister_function (entries are already gone) and does
  // NOT nil the Lua globals (they are still valid in the Lua state).
  // ============================================================================

  void LuaManager::restore_after_dll_reload() {
    if (!lua_) return;

    // The DLL reload wiped all blueprints; clear our tracking so re-running
    // the scripts can re-register them cleanly.
    StateMachineBlueprintRegistry::instance().clear_lua_blueprints();
    blueprint_source_files_.clear();

    // FunctionRegistry::clear_all() already removed the Lua entries, so just
    // reset the tracking set and let load_scripts() rebuild everything.
    lua_registered_functions_.clear();

    load_scripts();

    Log::info("[LuaManager] Lua state restored after DLL reload.");
  }

  // reload_scripts — hot-reload all Lua scripts and blueprints
  // ============================================================================

  void LuaManager::reload_scripts() {
    if (!lua_) return;

    Log::info("[LuaManager] Hot-reloading Lua scripts...");

    // 1. Remove all Lua-registered functions from FunctionRegistry
    auto& fr = FunctionRegistry::instance();
    using Sig0 = void();
    using Sig1 = void(ECS::Entity);
    using Sig2 = void(ECS::Entity, ECS::Entity);
    for (const auto& name : lua_registered_functions_) {
      fr.unregister_function<Sig0>(name);
      fr.unregister_function<Sig1>(name);
      fr.unregister_function<Sig2>(name);
    }
    lua_registered_functions_.clear();

    // 2. Remove all Lua-sourced blueprints and clear source file tracking
    StateMachineBlueprintRegistry::instance().clear_lua_blueprints();
    blueprint_source_files_.clear();

    // 3. Clear all Lua globals that are functions (non-builtin)
    // This prevents stale function references in the Lua state.
    // We need to be careful not to clear engine-bound tables.
    static const std::unordered_set<std::string> engine_globals = {
      "print", "type", "tostring", "tonumber", "pairs", "ipairs",
      "next", "select", "error", "pcall", "xpcall", "assert",
      "require", "rawget", "rawset", "rawequal", "rawlen",
      "setmetatable", "getmetatable", "load", "dofile", "loadfile",
      "collectgarbage", "coroutine", "string", "table", "math", "os",
      "io", "debug", "package", "utf8", "warn",
      // Engine tables and usertypes
      "ECS", "Log", "Input", "Key", "Time", "Audio", "Scene", "Level", "Entity",
      "Physics", "Systems", "Resources", "Upgrade", "Prefab", "Vec2D",
      "normalise_vector", "dot_product", "cross_product", "rotate", "scale_vector",
      // Engine-bound Lua functions (must survive reload)
      "register_blueprint"
    };

    // Collect function names to remove (can't modify table during iteration)
    std::vector<std::string> to_remove;
    sol::table globals = lua_->globals();
    globals.for_each([&](sol::object key_obj, sol::object val_obj) {
      if (key_obj.get_type() != sol::type::string) return;
      std::string key = key_obj.as<std::string>();
      if (engine_globals.count(key)) return;
      if (val_obj.get_type() == sol::type::function) {
        to_remove.push_back(key);
      }
    });
    for (auto& name : to_remove) {
      (*lua_)[name] = sol::lua_nil;
    }

    // 4. Re-load all scripts (re-registers functions and blueprints via register_blueprint())
    load_scripts();

    Log::info("[LuaManager] Hot-reload complete. {} Lua functions registered.",
              lua_registered_functions_.size());

#ifndef GAME_BUILD
    Engine::EventManager::send_event<Engine::Events::ToastEvent>(
      "Lua hot-reload complete",
      Engine::Events::ToastType::Info,
      3.0f
    );
#endif
  }

  // ============================================================================
  // File watcher — ReadDirectoryChangesW background thread
  // ============================================================================

  void LuaManager::start_file_watcher() {
#ifdef _WIN32
    if (watcher_running_) return;

    // Only start the watcher if the scripts directory exists
    if (!std::filesystem::exists("Assets/scripts")) {
      Log::info("[LuaManager] No Assets/scripts directory — file watcher not started.");
      return;
    }

    watcher_running_ = true;
    watcher_thread_ = std::thread(file_watcher_thread_fn, this);
    Log::info("[LuaManager] Lua file watcher started.");
#endif
  }

  void LuaManager::stop_file_watcher() {
#ifdef _WIN32
    if (!watcher_running_) return;
    watcher_running_ = false;
    if (watcher_thread_.joinable())
      watcher_thread_.join();
    Log::info("[LuaManager] Lua file watcher stopped.");
#endif
  }

  void LuaManager::file_watcher_thread_fn(LuaManager* self) {
#ifdef _WIN32
    std::filesystem::path watch_dir = std::filesystem::absolute("Assets/scripts");

    HANDLE dir_handle = CreateFileW(
      watch_dir.wstring().c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      nullptr);

    if (dir_handle == INVALID_HANDLE_VALUE) {
      Log::error("[LuaManager] Cannot watch scripts directory: {}", watch_dir.string());
      return;
    }

    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    OVERLAPPED overlapped{};
    overlapped.hEvent = event;

    alignas(DWORD) char buffer[4096];

    while (self->watcher_running_) {
      BOOL result = ReadDirectoryChangesW(
        dir_handle,
        buffer, sizeof(buffer),
        TRUE, // watch subtree (includes blueprints/)
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
        nullptr,
        &overlapped,
        nullptr);

      if (!result) break;

      // Wait with timeout so we can check watcher_running_ for clean shutdown
      DWORD wait = WaitForSingleObject(event, 500);
      if (wait == WAIT_OBJECT_0) {
        DWORD bytes_returned = 0;
        GetOverlappedResult(dir_handle, &overlapped, &bytes_returned, FALSE);
        ResetEvent(event);

        if (bytes_returned == 0) continue;

        // Parse the notification chain — check if any .lua file changed
        bool lua_changed = false;
        auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
        while (true) {
          std::wstring filename(info->FileName, info->FileNameLength / sizeof(WCHAR));
          if (filename.ends_with(L".lua")) {
            lua_changed = true;
            break;
          }
          if (info->NextEntryOffset == 0) break;
          info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
            reinterpret_cast<char*>(info) + info->NextEntryOffset);
        }

        if (lua_changed) {
          // Small debounce — wait for the editor to finish writing the file
          Sleep(300);
          self->lua_files_changed_ = true;
        }
      }
    }

    CloseHandle(event);
    CloseHandle(dir_handle);
#endif
  }

  void LuaManager::check_and_reload_if_changed() {
    if (!lua_) return;
    if (lua_files_changed_.exchange(false)) {
      Log::info("[LuaManager] Detected Lua file changes, reloading...");
      reload_scripts();
    }
  }

  // ============================================================================
  // create_lua_blueprint — create a new single-file Lua blueprint from template
  // ============================================================================

  std::string LuaManager::create_lua_blueprint(const std::string& blueprint_name) {
    std::filesystem::path scripts_dir = "Assets/scripts";
    std::filesystem::create_directories(scripts_dir);

    std::string snake = to_snake_case(blueprint_name);
    std::filesystem::path file_path = scripts_dir / (snake + ".lua");

    if (std::filesystem::exists(file_path)) {
      Log::warning("[LuaManager] Script file '{}' already exists.", file_path.string());
      return std::filesystem::absolute(file_path).string();
    }

    // Write a template single-file blueprint
    std::ofstream ofs(file_path);
    if (!ofs.is_open()) {
      Log::error("[LuaManager] Failed to create blueprint file '{}'.", file_path.string());
      return "";
    }

    ofs << "-- " << snake << ".lua\n";
    ofs << "-- State machine callbacks and blueprint for " << blueprint_name << "\n";
    ofs << "\n";
    ofs << "function " << snake << "_idle_on_enter(entity)\n";
    ofs << "  Log.info(\"" << blueprint_name << ": Idle enter\")\n";
    ofs << "end\n";
    ofs << "\n";
    ofs << "function " << snake << "_idle_on_update(entity)\n";
    ofs << "  -- TODO\n";
    ofs << "end\n";
    ofs << "\n";
    ofs << "function " << snake << "_idle_on_exit(entity)\n";
    ofs << "  -- TODO\n";
    ofs << "end\n";
    ofs << "\n";
    ofs << "register_blueprint(\"" << blueprint_name << "\", {\n";
    ofs << "  initial_state = \"Idle\",\n";
    ofs << "  states = {\n";
    ofs << "    { name = \"Idle\", on_enter = \"" << snake << "_idle_on_enter\", on_update = \"" << snake << "_idle_on_update\", on_exit = \"" << snake << "_idle_on_exit\" },\n";
    ofs << "  }\n";
    ofs << "})\n";

    ofs.close();

    Log::info("[LuaManager] Created Lua blueprint file '{}'.", file_path.string());

    // Reload scripts to pick up the new file
    reload_scripts();

    return std::filesystem::absolute(file_path).string();
  }

  // ============================================================================
  // delete_lua_blueprint — delete a blueprint file and unregister it
  // ============================================================================

  bool LuaManager::delete_lua_blueprint(const std::string& blueprint_name) {
    auto it = blueprint_source_files_.find(blueprint_name);
    if (it == blueprint_source_files_.end()) {
      Log::warning("[LuaManager] No source file tracked for blueprint '{}'.", blueprint_name);
      return false;
    }

    std::filesystem::path file_path(it->second);
    if (!std::filesystem::exists(file_path)) {
      Log::warning("[LuaManager] Blueprint file '{}' does not exist.", file_path.string());
      return false;
    }

    // Remove the file
    std::error_code ec;
    std::filesystem::remove(file_path, ec);
    if (ec) {
      Log::error("[LuaManager] Failed to delete blueprint file '{}': {}", file_path.string(), ec.message());
      return false;
    }

    Log::info("[LuaManager] Deleted Lua blueprint file '{}'.", file_path.string());

    // Reload scripts to clean up
    reload_scripts();

    return true;
  }

  // ============================================================================
  // open_lua_blueprint_in_editor — open the file in the system editor
  // ============================================================================

  void LuaManager::open_lua_blueprint_in_editor(const std::string& blueprint_name) {
    auto it = blueprint_source_files_.find(blueprint_name);
    if (it == blueprint_source_files_.end()) {
      Log::warning("[LuaManager] No source file tracked for blueprint '{}'.", blueprint_name);
      return;
    }

    std::filesystem::path file_path(it->second);
    if (!std::filesystem::exists(file_path)) {
      Log::warning("[LuaManager] Blueprint file '{}' does not exist.", file_path.string());
      return;
    }

    // Use ShellExecute on Windows to open with default editor
#ifdef _WIN32
    ShellExecuteA(NULL, "open", file_path.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    // Fallback for non-Windows platforms
    std::string cmd = "xdg-open \"" + file_path.string() + "\"";
    std::system(cmd.c_str());
#endif
  }

  // ============================================================================
  // get_lua_blueprint_path — get the file path for a Lua blueprint
  // ============================================================================

  std::string LuaManager::get_lua_blueprint_path(const std::string& blueprint_name) const {
    auto it = blueprint_source_files_.find(blueprint_name);
    if (it != blueprint_source_files_.end()) {
      return it->second;
    }
    return "";
  }

  // ============================================================================
  // add_state_to_lua_blueprint — insert stub callbacks + state entry into .lua
  // ============================================================================

  bool LuaManager::add_state_to_lua_blueprint(const std::string& bp_name,
                                               const std::string& state_name) {
    auto it = blueprint_source_files_.find(bp_name);
    if (it == blueprint_source_files_.end()) {
      Log::error("[LuaManager] add_state: no source file for blueprint '{}'.", bp_name);
      return false;
    }

    std::string file_path = it->second;

    // Read the file
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
      Log::error("[LuaManager] add_state: cannot open '{}'.", file_path);
      return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    ifs.close();

    std::string snake_prefix = to_snake_case(bp_name);
    std::string snake_state = to_lower(state_name);

    // Build stub function names
    std::string enter_fn  = snake_prefix + "_" + snake_state + "_on_enter";
    std::string update_fn = snake_prefix + "_" + snake_state + "_on_update";
    std::string exit_fn   = snake_prefix + "_" + snake_state + "_on_exit";

    // Build the three stub functions
    std::string stubs;
    stubs += "function " + enter_fn + "(entity)\n  -- TODO\nend\n\n";
    stubs += "function " + update_fn + "(entity)\n  -- TODO\nend\n\n";
    stubs += "function " + exit_fn + "(entity)\n  -- TODO\nend\n\n";

    // Build the state entry line
    std::string state_line = "    { name = \"" + state_name + "\", on_enter = \""
                           + enter_fn + "\", on_update = \"" + update_fn
                           + "\", on_exit = \"" + exit_fn + "\" },\n";

    // Find "register_blueprint" call to insert stubs before it
    size_t reg_pos = content.find("register_blueprint(");
    if (reg_pos == std::string::npos) {
      Log::error("[LuaManager] add_state: cannot find register_blueprint() in '{}'.", file_path);
      return false;
    }

    // Insert stubs before the register_blueprint call
    content.insert(reg_pos, stubs);

    // Now find the states table closing to insert the new state entry.
    // We look for the last state entry line (ends with "},") before the "})" closing.
    // Strategy: find "states = {" after register_blueprint, then find the last "},"
    // before the closing of the states table.
    size_t states_pos = content.find("states = {", reg_pos + stubs.size());
    if (states_pos == std::string::npos) {
      Log::error("[LuaManager] add_state: cannot find 'states = {{' in '{}'.", file_path);
      return false;
    }

    // Find the closing of the states array: first "  }" after "states = {"
    // We need to find the line with just "  }" or "  }," that closes the states array.
    // The safest approach: find "})" which closes register_blueprint, then search backwards
    // for the "}" that closes the states table.
    size_t reg_close = content.find("})", states_pos);
    if (reg_close == std::string::npos) {
      Log::error("[LuaManager] add_state: cannot find '}})' closing in '{}'.", file_path);
      return false;
    }

    // Search backwards from reg_close for "}" that closes the states array
    size_t states_close = content.rfind("}", reg_close - 1);
    if (states_close == std::string::npos || states_close <= states_pos) {
      Log::error("[LuaManager] add_state: cannot find states array closing in '{}'.", file_path);
      return false;
    }

    // Insert the new state entry just before the closing "}" of the states array
    content.insert(states_close, state_line);

    // Write back
    std::ofstream ofs(file_path);
    if (!ofs.is_open()) {
      Log::error("[LuaManager] add_state: cannot write '{}'.", file_path);
      return false;
    }
    ofs << content;
    ofs.close();

    Log::info("[LuaManager] Added state '{}' to blueprint '{}'.", state_name, bp_name);

    reload_scripts();
    return true;
  }

  // ============================================================================
  // remove_state_from_lua_blueprint — remove a state entry from the .lua file
  // (leaves callback functions intact for safety)
  // ============================================================================

  bool LuaManager::remove_state_from_lua_blueprint(const std::string& bp_name,
                                                    const std::string& state_name) {
    auto it = blueprint_source_files_.find(bp_name);
    if (it == blueprint_source_files_.end()) {
      Log::error("[LuaManager] remove_state: no source file for blueprint '{}'.", bp_name);
      return false;
    }

    std::string file_path = it->second;

    // Read the file
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
      Log::error("[LuaManager] remove_state: cannot open '{}'.", file_path);
      return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    ifs.close();

    // Find and remove the state entry line: { name = "StateName", ... },
    // We search for a pattern like: { name = "StateName"
    std::string search_pattern = "{ name = \"" + state_name + "\"";
    size_t line_start = content.find(search_pattern);
    if (line_start == std::string::npos) {
      Log::warning("[LuaManager] remove_state: state '{}' not found in '{}'.", state_name, file_path);
      return false;
    }

    // Find the beginning of this line (scan back to newline or start)
    size_t line_begin = content.rfind('\n', line_start);
    line_begin = (line_begin == std::string::npos) ? 0 : line_begin + 1;

    // Find the end of this line (scan forward to newline)
    size_t line_end = content.find('\n', line_start);
    if (line_end != std::string::npos) line_end += 1; // include the newline
    else line_end = content.size();

    content.erase(line_begin, line_end - line_begin);

    // Write back
    std::ofstream ofs(file_path);
    if (!ofs.is_open()) {
      Log::error("[LuaManager] remove_state: cannot write '{}'.", file_path);
      return false;
    }
    ofs << content;
    ofs.close();

    Log::info("[LuaManager] Removed state '{}' from blueprint '{}'.", state_name, bp_name);

    reload_scripts();
    return true;
  }

} // namespace Engine

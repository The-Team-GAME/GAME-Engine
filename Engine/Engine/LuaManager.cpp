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
#include "ECS/ECS.h"
#include "ECS/Entity.h"
#include "ECS/Reflection.h"
#include "ECS/Prefab.h"
#include "FunctionRegistry.h"
#include "Logger.h"
#include "InputManager.h"
#include "KeyCodes.h"
#include "Time.h"
#include "Audio/AudioSystem.h"
#include "SceneManager.h"
#include "LevelManager.h"
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
    bind_scene_manager();
    bind_level_manager();

    // 5. Load user scripts
    load_scripts();

    // 6. Load Lua-based state machine blueprints
    load_lua_blueprints();

    // 7. Start background file watcher for hot-reload
    start_file_watcher();

    Log::info("[LuaManager] Initialisation complete. {} component types bound.",
              impl_->component_getters.size());
  }

  // ============================================================================
  // shutdown
  // ============================================================================

  void LuaManager::shutdown() {
    stop_file_watcher();

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

    // Pass 1: bind component usertypes and store getters
    for (auto& entry : reg.entries()) {
      // Bind the component's usertype (recursively binds field types too)
      entry.binder(*lua_);

      // Store the component getter for the Entity __index metamethod
      impl_->component_getters[entry.short_name] = entry.getter;

      Log::info("[LuaManager] Bound component '{}'", entry.short_name);
    }

    // Pass 2: augment usertypes with extra methods (from LUA_BIND_METHODS)
    for (auto& entry : reg.method_binder_entries()) {
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
    res["load_texture"] = [](const std::string& path) {
      return Resources::load_texture(path);
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

    // Mouse position — returns x, y as two values
    input["get_mouse_position"] = [](sol::this_state ts) -> sol::object {
      auto pos = Engine::Input::get_mouse_position();
      sol::state_view lua(ts);
      sol::table t = lua.create_table();
      t["x"] = pos.x;
      t["y"] = pos.y;
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
  // load_scripts — execute every .lua file in Assets/scripts/ (non-recursive,
  //                skips the blueprints/ subdirectory which is handled separately)
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
      auto result = L.safe_script_file(path_str, sol::script_pass_on_error);
      if (!result.valid()) {
        sol::error err = result;
        Log::error("[LuaManager] Error loading '{}': {}", path_str, err.what());
        continue;
      }

      Log::info("[LuaManager] Loaded script '{}'", file_path.filename().string());
    }

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
      using Sig1 = void(ECS::Entity);
      using Sig2 = void(ECS::Entity, ECS::Entity);
      if (fr.has_function<Sig1>(func_name) && lua_registered_functions_.count(func_name) == 0) return;

      // Capture the Lua function and wrap it
      sol::protected_function lua_func = val_obj.as<sol::protected_function>();
      std::string captured_name = func_name; // capture for lambda

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
  // load_lua_blueprints — parse Blueprint tables from Assets/scripts/blueprints/
  // ============================================================================

  void LuaManager::load_lua_blueprints() {
    auto& L = *lua_;

    std::filesystem::path bp_dir = "Assets/scripts/blueprints";
    if (!std::filesystem::exists(bp_dir)) {
      Log::info("[LuaManager] No blueprints directory found at '{}'. Skipping.", bp_dir.string());
      return;
    }

    auto& bp_registry = StateMachineBlueprintRegistry::instance();

    for (auto& entry : std::filesystem::directory_iterator(bp_dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;

      // The blueprint name is the filename without extension
      std::string bp_name = entry.path().stem().string();

      // The Blueprint global might have been overwritten by the last loaded script.
      // We need a per-file approach: set Blueprint to nil, re-run the file,
      // then read it.
      L["Blueprint"] = sol::lua_nil;
      auto result = L.safe_script_file(entry.path().string(), sol::script_pass_on_error);
      if (!result.valid()) {
        sol::error err = result;
        Log::error("[LuaManager] Error loading blueprint '{}': {}", bp_name, err.what());
        continue;
      }

      sol::optional<sol::table> bp_table = L["Blueprint"].get<sol::optional<sol::table>>();
      if (!bp_table) {
        // Not a blueprint file (no Blueprint table defined) — that's fine, skip it
        continue;
      }

      // Parse the Blueprint table
      StateMachineBlueprint blueprint;

      sol::optional<std::string> initial = (*bp_table)["initial_state"];
      if (initial) {
        blueprint.initial_state = *initial;
      }

      sol::optional<sol::table> states_table = (*bp_table)["states"];
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

      // Clear the Blueprint global so it doesn't leak to the next file
      L["Blueprint"] = sol::lua_nil;

      if (blueprint.states.empty()) {
        Log::warning("[LuaManager] Blueprint '{}' has no states, skipping.", bp_name);
        continue;
      }

      bp_registry.register_lua_blueprint(bp_name, blueprint);
      Log::info("[LuaManager] Registered Lua blueprint '{}' with {} states.",
                bp_name, blueprint.states.size());
    }
  }

  // ============================================================================
  // reload_scripts — hot-reload all Lua scripts and blueprints
  // ============================================================================

  void LuaManager::reload_scripts() {
    if (!lua_) return;

    Log::info("[LuaManager] Hot-reloading Lua scripts...");

    // 1. Remove all Lua-registered functions from FunctionRegistry
    auto& fr = FunctionRegistry::instance();
    using Sig1 = void(ECS::Entity);
    using Sig2 = void(ECS::Entity, ECS::Entity);
    for (const auto& name : lua_registered_functions_) {
      fr.unregister_function<Sig1>(name);
      fr.unregister_function<Sig2>(name);
    }
    lua_registered_functions_.clear();

    // 2. Remove all Lua-sourced blueprints
    StateMachineBlueprintRegistry::instance().clear_lua_blueprints();

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
      "normalise_vector", "dot_product", "cross_product", "rotate", "scale_vector"
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

    // Also clear the Blueprint global
    (*lua_)["Blueprint"] = sol::lua_nil;

    // 4. Re-load all scripts (this re-registers functions)
    load_scripts();

    // 5. Re-load Lua blueprints
    load_lua_blueprints();

    Log::info("[LuaManager] Hot-reload complete. {} Lua functions registered.",
              lua_registered_functions_.size());
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
  // create_lua_blueprint — create a new blueprint .lua file from template
  // ============================================================================

  std::string LuaManager::create_lua_blueprint(const std::string& blueprint_name) {
    std::filesystem::path bp_dir = "Assets/scripts/blueprints";
    std::filesystem::create_directories(bp_dir);

    std::filesystem::path file_path = bp_dir / (blueprint_name + ".lua");
    if (std::filesystem::exists(file_path)) {
      Log::warning("[LuaManager] Blueprint file '{}' already exists.", file_path.string());
      return file_path.string();
    }

    // Write a template blueprint file
    std::ofstream ofs(file_path);
    if (!ofs.is_open()) {
      Log::error("[LuaManager] Failed to create blueprint file '{}'.", file_path.string());
      return "";
    }

    ofs << "-- " << blueprint_name << " Blueprint\n";
    ofs << "-- Define callback functions for each state below.\n";
    ofs << "-- These functions receive an Entity parameter.\n";
    ofs << "\n";
    ofs << "function " << blueprint_name << "_idle_enter(entity)\n";
    ofs << "  Log.info(\"" << blueprint_name << ": Idle enter\")\n";
    ofs << "end\n";
    ofs << "\n";
    ofs << "function " << blueprint_name << "_idle_update(entity)\n";
    ofs << "  -- Update logic here\n";
    ofs << "end\n";
    ofs << "\n";
    ofs << "-- Blueprint definition table\n";
    ofs << "Blueprint = {\n";
    ofs << "  initial_state = \"Idle\",\n";
    ofs << "  states = {\n";
    ofs << "    { name = \"Idle\", on_enter = \"" << blueprint_name << "_idle_enter\", on_update = \"" << blueprint_name << "_idle_update\", on_exit = \"\" },\n";
    ofs << "  }\n";
    ofs << "}\n";

    ofs.close();

    Log::info("[LuaManager] Created Lua blueprint file '{}'.", file_path.string());

    // Reload scripts to pick up the new file
    reload_scripts();

    return file_path.string();
  }

  // ============================================================================
  // delete_lua_blueprint — delete a blueprint file and unregister it
  // ============================================================================

  bool LuaManager::delete_lua_blueprint(const std::string& blueprint_name) {
    std::filesystem::path file_path = "Assets/scripts/blueprints" / std::filesystem::path(blueprint_name + ".lua");

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
    std::filesystem::path file_path = std::filesystem::absolute("Assets/scripts/blueprints" / std::filesystem::path(blueprint_name + ".lua"));

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
    std::filesystem::path file_path = "Assets/scripts/blueprints" / std::filesystem::path(blueprint_name + ".lua");
    if (std::filesystem::exists(file_path)) {
      return std::filesystem::absolute(file_path).string();
    }
    return "";
  }

} // namespace Engine

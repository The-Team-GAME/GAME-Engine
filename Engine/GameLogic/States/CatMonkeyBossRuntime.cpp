//-----------------------------------------------------------------------------------
// file:    CatMonkeyBossRuntime.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Runtime logic and helpers for Cat Monkey boss fight.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include <random>
#include "CatMonkeyBossRuntime.h"

namespace CatMonkeyStateCallbacks
{
  namespace
  {
    static constexpr const char* k_player_camera_name = "Player_Camera";

    // City layer names/prefabs (extracted from original)
    static constexpr const char* k_cityscape_layer_name = "cityscape";
    static constexpr const char* k_foreground_city_layer_name = "foreground_city";
    static constexpr const char* k_background_city_layer_name = "background_city";

    static constexpr const char* k_cityscape_prefab_path = "prefabs/cityscape.json";
    static constexpr const char* k_foreground_city_prefab_path = "prefabs/foreground_city.json";
    static constexpr const char* k_background_city_prefab_path = "prefabs/background_city.json";

    static constexpr float k_cityscape_speed = 200.0f;
    static constexpr float k_foreground_city_speed = 180.0f;
    static constexpr float k_background_city_speed = 350.0f;

    // Mid-building oscillation
    static constexpr const char* k_mid_building_name   = "mid_building";
    static constexpr const char* k_mid_building_prefab = "prefabs/mid_building.json";
    static constexpr float k_mid_building_start_x      = 1000.0f;
    static constexpr float k_mid_building_start_y      = 780.0f;
    static constexpr float k_mid_building_amplitude    = 100.0f; // oscillates ±100 from start_x
    static constexpr float k_mid_building_speed        = 0.3f;   // radians/sec (slow sway)

    // Ambient camera shake
    static constexpr float k_ambient_shake_amplitude   = 0.25f;  // y offset in world units
    static constexpr float k_ambient_shake_interval_min = 4.0f;  // seconds between shakes
    static constexpr float k_ambient_shake_interval_max = 10.0f;
    static constexpr float k_ambient_shake_tick_sec    = 0.05f;  // time per sub-tick
    static constexpr int   k_ambient_shake_ticks       = 6;      // ticks per shake burst

    static constexpr const char* k_city_layer_blueprint = "CatMonkeyCityLayer";
    static constexpr const char* k_city_layer_active_state = "Active";
  }

  std::unordered_map<Engine::ECS::EntityID, BossRuntime>& runtimes()
  {
    static std::unordered_map<Engine::ECS::EntityID, BossRuntime> map;
    return map;
  }

  BossRuntime& runtime_for(Engine::ECS::Entity boss)
  {
    return runtimes()[boss.get_id()];
  }

  void clear_runtime_for(Engine::ECS::Entity boss)
  {
    runtimes().erase(boss.get_id());
  }

  Engine::ECS::Entity find_camera_entity_by_name(const char* name)
  {
    Engine::ECS::Entity found;
    if (!name || *name == '\0') {
      return found;
    }

    Engine::ECS::for_each_entity_with<Components::Camera>(
      [&](Engine::ECS::Entity e)
      {
        if (found.is_valid()) {
          return;
        }
        if (e.get_name() == name) {
          found = e;
        }
      });

    return found;
  }

  void set_active_camera_entity(Engine::ECS::Entity camera_entity)
  {
    if (!camera_entity.is_valid()) {
      return;
    }

    Engine::ECS::for_each_entity_with<Components::Camera>(
      [&](Engine::ECS::Entity e)
      {
        if (auto* cam = e.get_component<Components::Camera>()) {
          cam->is_active = false;
        }
      });

    if (auto* cam = camera_entity.get_component<Components::Camera>()) {
      cam->is_active = true;
    }
  }

  void activate_player_camera_if_present()
  {
    Engine::ECS::Entity cam = find_camera_entity_by_name(k_player_camera_name);
    if (cam.is_valid()) {
      set_active_camera_entity(cam);
    }
  }

  void activate_cat_monkey_camera_if_present()
  {
    Engine::ECS::Entity cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
    if (cam.is_valid()) {
      set_active_camera_entity(cam);
    }
  }

  void activate_centered_camera_if_present()
  {
    Engine::ECS::Entity cam = find_camera_entity_by_name(k_centered_camera_name);
    if (cam.is_valid()) {
      set_active_camera_entity(cam);
    }
  }

  Engine::ECS::Entity find_entity_by_name_with_transform(const char* name)
  {
    Engine::ECS::Entity found;
    if (!name || *name == '\0') {
      return found;
    }

    Engine::ECS::for_each_entity_with<Components::Transform2D>(
      [&](Engine::ECS::Entity e)
      {
        if (found.is_valid()) {
          return;
        }
        if (e.get_name() == name) {
          found = e;
        }
      });

    return found;
  }

  void set_entity_alpha_if_present(Engine::ECS::Entity entity, float alpha)
  {
    if (auto* color = entity.get_component<Components::Color>()) {
      color->a = std::clamp(alpha, 0.0f, 1.0f);
    }
  }

  void detach_from_parent_if_present(Engine::ECS::Entity entity)
  {
    if (entity.is_valid() && entity.has_component<Components::Parent>()) {
      entity.remove_component<Components::Parent>();
    }
  }

  void detach_centered_camera_parent_for_cinematic(BossRuntime& rt)
  {
    Engine::ECS::Entity cam = find_camera_entity_by_name(k_centered_camera_name);
    if (!cam.is_valid()) return;

    auto* parent = cam.get_component<Components::Parent>();
    if (!rt.centered_cam_parent_saved) {
      rt.centered_cam_parent_saved = true;
      rt.centered_cam_had_parent = (parent != nullptr);
      if (parent) {
        rt.centered_cam_saved_parent = *parent;
      }
      rt.centered_cam_parent_added = false;
    }

    // For cinematics we want the same fixed Centered camera behavior as Summon1-4.
    // If the level has parented this camera (e.g., to the player), detach it temporarily
    // so it doesn't drift.
    if (parent) {
      cam.remove_component<Components::Parent>();
    }
  }

  void restore_centered_camera_parent(BossRuntime& rt)
  {
    if (!rt.centered_cam_parent_saved) return;

    Engine::ECS::Entity cam = find_camera_entity_by_name(k_centered_camera_name);
    if (!cam.is_valid()) {
      rt.centered_cam_parent_saved = false;
      return;
    }

    if (!rt.centered_cam_had_parent) {
      if (rt.centered_cam_parent_added && cam.has_component<Components::Parent>()) {
        cam.remove_component<Components::Parent>();
      }
    } else {
      auto* parent = cam.get_component<Components::Parent>();
      if (!parent) {
        cam.add_component<Components::Parent>();
        parent = cam.get_component<Components::Parent>();
      }
      if (parent) {
        *parent = rt.centered_cam_saved_parent;
      }
    }

    rt.centered_cam_parent_saved = false;
  }

  void ensure_city_layers_hidden(BossRuntime& rt)
  {
    if (rt.city_layers_hidden) {
      return;
    }

    set_entity_alpha_if_present(find_entity_by_name_with_transform(k_cityscape_layer_name), 0.0f);
    set_entity_alpha_if_present(find_entity_by_name_with_transform(k_foreground_city_layer_name), 0.0f);
    set_entity_alpha_if_present(find_entity_by_name_with_transform(k_background_city_layer_name), 0.0f);
    rt.city_layers_hidden = true;
  }

  inline Engine::ECS::Entity ensure_layer(const char* prefab_path, const char* entity_name, int index) {
    std::string name = entity_name;
    if (index > 0) name += "_copy";
    Engine::ECS::Entity e = find_entity_by_name_with_transform(name.c_str());
    if (!e.is_valid()) {
      Engine::ECS::Prefab prefab = Engine::Resources::load_prefab(prefab_path);
      if (prefab.is_valid()) {
        e = Engine::ECS::instantiate_prefab(prefab);
        if (e.is_valid()) {
          Engine::ECS::set_entity_name(e, name.c_str());
        }
      }
    }
    return e;
  }

  inline void ensure_city_layer_state_machine(Engine::ECS::Entity e)
  {
    if (!e.is_valid()) {
      return;
    }

    auto* sm = e.get_component<Components::StateMachine>();
    if (!sm) {
      e.add_component<Components::StateMachine>();
      sm = e.get_component<Components::StateMachine>();
    }
    if (!sm) {
      return;
    }

    if (!sm->has_state(k_city_layer_active_state)) {
      sm->add_state(Components::State(k_city_layer_active_state));
    }

    sm->blueprint_id = k_city_layer_blueprint;

    if (sm->get_current_state().empty()) {
      sm->set_initial_state(k_city_layer_active_state);
    }
    else if (!sm->is_in_state(k_city_layer_active_state)) {
      sm->change_state(k_city_layer_active_state);
    }

    sm->play_property_anim();
  }

  inline void init_city_scroll_if_needed(BossRuntime& rt) {
    if (!rt.city_scroll_enabled || rt.city_scroll_initialized) return;

    if (!rt.debug_logged_city_init) {
      Log::info("CatMonkey: initializing Summon5 city scroll.");
      rt.debug_logged_city_init = true;
    }

    auto setup_single = [&](const char* prefab_path, const char* base_name, float start_x, float start_y) {
      Engine::ECS::Entity a = ensure_layer(prefab_path, base_name, 0);
      detach_from_parent_if_present(a);
      set_entity_alpha_if_present(a, 1.0f);
      ensure_city_layer_state_machine(a);
      
      if (auto* ta = a.get_component<Components::Transform2D>()) {
        ta->position.x = start_x;
        ta->position.y = start_y;
      }
      
      // Cleanup copy if it somehow existed previously
      std::string copy_name = std::string(base_name) + "_copy";
      Engine::ECS::Entity b = find_entity_by_name_with_transform(copy_name.c_str());
      if (b.is_valid()) {
          b.destroy();
      }
    };

    setup_single(k_cityscape_prefab_path, k_cityscape_layer_name, 6000.0f, 300.0f);
    setup_single(k_foreground_city_prefab_path, k_foreground_city_layer_name, 4500.0f, 25.0f);
    setup_single(k_background_city_prefab_path, k_background_city_layer_name, 4500.0f, 0.0f);

    rt.city_scroll_initialized = true;
  }

  void tick_city_scroll_if_enabled(BossRuntime& rt)
  {
    if (!rt.city_scroll_enabled) return;

    init_city_scroll_if_needed(rt);
    if (!rt.city_scroll_initialized) return;

    const float dt = Engine::Time::get_fixed_dt();
    if (dt <= 0.0f) return;

    auto tick_single = [&](const char* base_name, float speed, float end_x, float start_x) {
      Engine::ECS::Entity a = find_entity_by_name_with_transform(base_name);
      if (!a.is_valid()) return;

      auto timeline_is_driving = [](Engine::ECS::Entity e) {
        auto* sm = e.get_component<Components::StateMachine>();
        return sm && sm->is_property_anim_playing();
      };

      if (timeline_is_driving(a)) {
        return;
      }

      auto* ta = a.get_component<Components::Transform2D>();
      if (!ta) return;
      
      // Move left (negative speed)
      ta->position.x -= speed * dt;
      
      if (ta->position.x <= end_x) {
          ta->position.x = start_x;
      }
    };

    tick_single(k_cityscape_layer_name, k_cityscape_speed, -5000.0f, 6000.0f);
    tick_single(k_foreground_city_layer_name, k_foreground_city_speed, -3500.0f, 4500.0f);
    tick_single(k_background_city_layer_name, k_background_city_speed, -2500.0f, 4500.0f);
  }

  void tick_mid_building_if_active(BossRuntime& rt)
  {
    if (!rt.mid_building_enabled) return;

    // First-time init: find or spawn the entity and snap it to start position.
    if (!rt.mid_building_initialized) {
      Engine::ECS::Entity e = find_entity_by_name_with_transform(k_mid_building_name);
      if (!e.is_valid()) {
        e = Engine::ECS::instantiate_prefab(Engine::Resources::load_prefab(k_mid_building_prefab));
      }
      if (e.is_valid()) {
        if (auto* t = e.get_component<Components::Transform2D>()) {
          t->position.x = k_mid_building_start_x;
          t->position.y = k_mid_building_start_y;
        }
        if (auto* c = e.get_component<Components::Color>()) {
          c->a = 1.0f;
        }
      }
      rt.mid_building_phase = 0.0f;
      rt.mid_building_initialized = true;
    }

    const float dt = Engine::Time::get_fixed_dt();
    if (dt <= 0.0f) return;

    rt.mid_building_phase += k_mid_building_speed * dt;

    Engine::ECS::Entity e = find_entity_by_name_with_transform(k_mid_building_name);
    if (!e.is_valid()) return;
    auto* t = e.get_component<Components::Transform2D>();
    if (!t) return;

    // Sinusoidal oscillation: start_x + amplitude * sin(phase)
    // sin goes 0 -> -1 -> 0 -> +1 -> 0, matching left-right-centre-right-left cycle.
    t->position.x = k_mid_building_start_x + k_mid_building_amplitude * std::sin(rt.mid_building_phase);
    t->position.y = k_mid_building_start_y;
  }

  void tick_ambient_camera_shake(BossRuntime& rt)
  {
    if (!rt.ambient_shake_active) return;

    const float now = Engine::Time::get_time();

    // Between shakes: wait until next scheduled time.
    if (rt.ambient_shake_tick == 0 && now < rt.ambient_shake_next_time) return;

    // Determine which camera is currently active.
    Engine::ECS::Entity cam;
    if (rt.camera_override_active && rt.camera_override_mode == BossRuntime::CameraOverrideMode::CatMonkey) {
      cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
    } else {
      cam = find_camera_entity_by_name(k_centered_camera_name);
    }
    if (!cam.is_valid()) return;
    auto* t = cam.get_component<Components::Transform2D>();
    if (!t) return;

    // Fire first sub-tick when time arrives.
    if (rt.ambient_shake_tick == 0) {
      rt.ambient_shake_tick = 1;
      rt.ambient_shake_tick_time = now + k_ambient_shake_tick_sec;
      rt.ambient_shake_offset_y = k_ambient_shake_amplitude;
      t->position.y += rt.ambient_shake_offset_y;
      return;
    }

    // Advance sub-ticks.
    if (now < rt.ambient_shake_tick_time) return;
    rt.ambient_shake_tick_time = now + k_ambient_shake_tick_sec;

    // Remove previous offset then apply new one (alternating up/down).
    t->position.y -= rt.ambient_shake_offset_y;
    rt.ambient_shake_tick++;

    if (rt.ambient_shake_tick > k_ambient_shake_ticks) {
      // Shake burst done; ensure y is restored and schedule next shake.
      rt.ambient_shake_offset_y = 0.0f;
      rt.ambient_shake_tick = 0;

      // Random interval [min, max]
      static std::mt19937 rng{ std::random_device{}() };
      std::uniform_real_distribution<float> dist(k_ambient_shake_interval_min, k_ambient_shake_interval_max);
      rt.ambient_shake_next_time = now + dist(rng);
      return;
    }

    // Alternate direction each sub-tick.
    rt.ambient_shake_offset_y = (rt.ambient_shake_tick % 2 == 0)
      ? k_ambient_shake_amplitude : -k_ambient_shake_amplitude;
    t->position.y += rt.ambient_shake_offset_y;
  }

  void start_summon5_cat_camera_shake(BossRuntime& rt)
  {
    Engine::ECS::Entity cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
    if (!cam.is_valid()) {
      rt.summon5_shake_active = false;
      return;
    }

    auto* t = cam.get_component<Components::Transform2D>();
    if (!t) {
      rt.summon5_shake_active = false;
      return;
    }

    rt.summon5_shake_active = true;
    rt.summon5_shakes_done = 0;
    rt.summon5_next_shake_time = Engine::Time::get_time();
    rt.summon5_cat_cam_base_pos = t->position;
    rt.summon5_cat_cam_base_rot = t->rotation;
  }

  void tick_summon5_cat_camera_shake(BossRuntime& rt)
  {
    if (!rt.summon5_shake_active) return;

    Engine::ECS::Entity cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
    if (!cam.is_valid()) {
      rt.summon5_shake_active = false;
      return;
    }

    auto* t = cam.get_component<Components::Transform2D>();
    if (!t) {
      rt.summon5_shake_active = false;
      return;
    }

    const float now = Engine::Time::get_time();
    if (now < rt.summon5_next_shake_time) return;

    if (rt.summon5_shakes_done >= k_summon5_shake_count) {
      t->position = rt.summon5_cat_cam_base_pos;
      t->rotation = rt.summon5_cat_cam_base_rot;
      rt.summon5_shake_active = false;
      return;
    }

    Math::Vec2D offset{};
    const int phase = rt.summon5_shakes_done % 4;
    if (phase == 0) offset = { k_summon5_shake_amplitude, 0.0f };
    else if (phase == 1) offset = { -k_summon5_shake_amplitude, 0.0f };
    else if (phase == 2) offset = { 0.0f, k_summon5_shake_amplitude };
    else offset = { 0.0f, -k_summon5_shake_amplitude };

    t->position = rt.summon5_cat_cam_base_pos + offset;
    rt.summon5_shakes_done++;
    rt.summon5_next_shake_time = now + k_summon5_shake_interval_sec;
  }

  void tick_post_spawn_cinematic_camera(BossRuntime& rt)
  {
    if (!rt.post_spawn_cinematic_active) return;

    if (rt.pending_summon_immediate_zoom) return;

    const float now = Engine::Time::get_time();
    Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
    if (!centered.is_valid()) {
      if (!rt.debug_logged_centered_camera_missing) {
        Log::warning("CatMonkey: post-spawn cinematic active but '{}' not found.", k_centered_camera_name);
        rt.debug_logged_centered_camera_missing = true;
      }
      return;
    }

    auto* t = centered.get_component<Components::Transform2D>();
    if (!t) return;

    // If we've passed the end, snap to the final state once and stop.
    if (now >= rt.post_spawn_cinematic_end_time) {
      const float target_zoom = std::clamp(rt.post_spawn_zoom_target, 0.1f, 10.0f);
      if (!centered.has_component<Components::Parent>()) {
        t->position = rt.post_spawn_pos_target;
        t->rotation = rt.post_spawn_rot_target;
      }
      t->scale = { target_zoom, target_zoom };
      rt.post_spawn_cinematic_active = false;
      return;
    }

    const float dur = std::max(0.001f, rt.post_spawn_cinematic_end_time - rt.post_spawn_cinematic_start_time);
    float u = std::clamp((now - rt.post_spawn_cinematic_start_time) / dur, 0.0f, 1.0f);
    // smootherstep (smoother than smoothstep)
    u = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);

    const float target_zoom = std::clamp(rt.post_spawn_zoom_target, 0.1f, 10.0f);
    const float z = rt.post_spawn_zoom_start + (target_zoom - rt.post_spawn_zoom_start) * u;

    // If the camera is parented (e.g., temporarily to the boss), don't fight the hierarchy system.
    // Keep it centered by only adjusting zoom.
    if (!centered.has_component<Components::Parent>()) {
      t->position = rt.post_spawn_pos_start + (rt.post_spawn_pos_target - rt.post_spawn_pos_start) * u;
      t->rotation = rt.post_spawn_rot_start + (rt.post_spawn_rot_target - rt.post_spawn_rot_start) * u;
    }
    t->scale = {z, z};
  }
}


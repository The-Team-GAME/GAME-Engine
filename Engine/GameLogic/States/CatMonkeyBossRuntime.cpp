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
#include "CatMonkeyBossRuntime.h"

namespace CatMonkeyStateCallbacks
{
  namespace
  {
    static constexpr const char* k_player_camera_name = "Player_Camera";

    // City layer names/prefabs (extracted from original)
    static constexpr const char* k_cityscape_layer_name = "cityscape";
    static constexpr const char* k_mid_building_layer_name = "mid_building";
    static constexpr const char* k_minicity_layer_name = "minicity";
    static constexpr const char* k_foreground_city_layer_name = "foreground_city";
    static constexpr const char* k_background_city_layer_name = "background_city";

    static constexpr const char* k_cityscape_prefab_path = "prefabs/cityscape.json";
    static constexpr const char* k_mid_building_prefab_path = "prefabs/mid_building.json";
    static constexpr const char* k_foreground_city_prefab_path = "prefabs/foreground_city.json";
    static constexpr const char* k_background_city_prefab_path = "prefabs/background_city.json";

    static constexpr float k_cityscape_speed = 200.0f;
    static constexpr float k_minicity_speed = 200.0f;
    static constexpr float k_foreground_city_speed = 180.0f;
    static constexpr float k_background_city_speed = 350.0f;

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
    set_entity_alpha_if_present(find_entity_by_name_with_transform(k_mid_building_layer_name), 0.0f);
    set_entity_alpha_if_present(find_entity_by_name_with_transform(k_minicity_layer_name), 0.0f);
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

    const Engine::Graphics::CameraBounds bounds = Engine::Graphics::get_camera_bounds();
    const float cam_left = bounds.min_x;
    const float cam_bottom = bounds.min_y;

    if (!rt.debug_logged_city_init) {
      Log::info("CatMonkey: initializing Summon5 city scroll. Camera bounds min_x={}, max_x={}", cam_left, bounds.max_x);
      rt.debug_logged_city_init = true;
    }

    auto setup_pair = [&](const char* prefab_path, const char* base_name) {
      Engine::ECS::Entity a = ensure_layer(prefab_path, base_name, 0);
      Engine::ECS::Entity b = ensure_layer(prefab_path, base_name, 1);
      for (Engine::ECS::Entity e : {a, b}) {
        detach_from_parent_if_present(e);
        set_entity_alpha_if_present(e, 1.0f);
        ensure_city_layer_state_machine(e);
        if (auto* t = e.get_component<Components::Transform2D>()) {
          // Align all layers by their bottom-left corner.
          t->position.y = cam_bottom + 0.5f * t->scale.y;
        }
      }
      if (auto* ta = a.get_component<Components::Transform2D>()) {
        const float w = std::max(1.0f, ta->scale.x);
        ta->position.x = cam_left + 0.5f * w;
        if (auto* tb = b.get_component<Components::Transform2D>()) {
          tb->position.x = ta->position.x + w;
        }
      }
    };

    setup_pair(k_cityscape_prefab_path, k_cityscape_layer_name);
    setup_pair(k_mid_building_prefab_path, k_mid_building_layer_name);
    setup_pair(k_foreground_city_prefab_path, k_foreground_city_layer_name);
    setup_pair(k_background_city_prefab_path, k_background_city_layer_name);

    rt.city_scroll_initialized = true;
  }

  void tick_city_scroll_if_enabled(BossRuntime& rt)
  {
    if (!rt.city_scroll_enabled) return;

    init_city_scroll_if_needed(rt);
    if (!rt.city_scroll_initialized) return;

    const Engine::Graphics::CameraBounds bounds = Engine::Graphics::get_camera_bounds();
    const float dt = Engine::Time::get_fixed_dt();
    if (dt <= 0.0f) return;

    auto tick_pair = [&](const char* base_name, float speed) {
      std::string copy_name = std::string(base_name) + "_copy";
      Engine::ECS::Entity a = find_entity_by_name_with_transform(base_name);
      Engine::ECS::Entity b = find_entity_by_name_with_transform(copy_name.c_str());
      if (!a.is_valid() || !b.is_valid()) return;

      auto timeline_is_driving = [](Engine::ECS::Entity e) {
        auto* sm = e.get_component<Components::StateMachine>();
        return sm && sm->is_property_anim_playing();
      };

      // If either layer is currently driven by its timeline clip, don't fight it.
      if (timeline_is_driving(a) || timeline_is_driving(b)) {
        return;
      }

      auto* ta = a.get_component<Components::Transform2D>();
      auto* tb = b.get_component<Components::Transform2D>();
      if (!ta || !tb) return;
      float width = std::max(ta->scale.x, tb->scale.x);
      ta->position.x += speed * dt;
      tb->position.x += speed * dt;
      // Loop seamlessly when a layer's left edge reaches the camera right edge.
      if (ta->position.x - 0.5f * ta->scale.x >= bounds.max_x) ta->position.x = tb->position.x + width;
      if (tb->position.x - 0.5f * tb->scale.x >= bounds.max_x) tb->position.x = ta->position.x + width;
    };

    tick_pair(k_cityscape_layer_name, k_cityscape_speed);
    tick_pair(k_mid_building_layer_name, k_minicity_speed);
    tick_pair(k_foreground_city_layer_name, k_foreground_city_speed);
    tick_pair(k_background_city_layer_name, k_background_city_speed);
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


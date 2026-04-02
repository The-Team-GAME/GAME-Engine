//-----------------------------------------------------------------------------------
// file:    CatMonkeyStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   State machine logic and helpers for Cat Monkey boss fight.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include <random>
#include "CatMonkeyStateCallbacks.h"
#include "CatMonkeyBossRuntime.h"
#include "CatMonkeyTunnelLogic.h"
#include "CatMonkeyTrainTracksLogic.h"
#include "../Components/EnemyHealthComponent.h"
#include "../Components/CatMonkeyBossComponent.h"
#include "../Components/CatMonkeySummonWaveHealthBarsComponent.h"
#include "../Components/CatMonkeySummonHealthBarComponent.h"
using namespace CatMonkeyStateCallbacks;

namespace CatMonkeyStateCallbacks
{
  inline void ensure_centered_camera_baseline(BossRuntime& rt)
  {
    if (rt.centered_cam_baseline_saved) return;

    Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
    if (!centered.is_valid()) return;
    if (auto* t = centered.get_component<Components::Transform2D>()) {
      rt.centered_cam_baseline_saved = true;
      rt.centered_cam_baseline_pos = t->position;
      rt.centered_cam_baseline_rot = t->rotation;
      rt.centered_cam_baseline_zoom = t->scale.x;
    }
  }

  inline void apply_centered_camera_baseline(BossRuntime& rt)
  {
    if (!rt.centered_cam_baseline_saved) return;
    Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
    if (!centered.is_valid()) return;
    if (auto* t = centered.get_component<Components::Transform2D>()) {
      t->position = rt.centered_cam_baseline_pos;
      t->rotation = rt.centered_cam_baseline_rot;
      const float z = std::clamp(rt.centered_cam_baseline_zoom, 0.1f, 10.0f);
      t->scale = {z, z};
    }
  }

  // Internal helpers (healthbar, animation)
  inline const Components::CatMonkeyBoss* get_cfg(Engine::ECS::Entity boss) {
    return boss.get_component<Components::CatMonkeyBoss>();
  }

  bool is_ai_disabled(Engine::ECS::Entity boss) {
    if (const auto* cfg = get_cfg(boss)) return cfg->ai_disabled;
    return false;
  }

  int minions_per_summon_for(Engine::ECS::Entity boss, int summon_index) {
    const auto* cfg = get_cfg(boss);
    if (!cfg) return 0;

    int per_wave = 0;
    switch (summon_index) {
      case 0: per_wave = cfg->summon1_minions; break;
      case 1: per_wave = cfg->summon2_minions; break;
      case 2: per_wave = cfg->summon3_minions; break;
      case 3: per_wave = cfg->summon4_minions; break;
      case 4: per_wave = cfg->summon5_minions; break;
    }
    return per_wave > 0 ? per_wave : cfg->minions_per_summon;
  }

  float summon_cooldown(Engine::ECS::Entity boss) {
    if (const auto* cfg = get_cfg(boss)) return cfg->summon_cooldown;
    return 2.0f;
  }

  inline void restart_entity_animation(Engine::ECS::Entity entity) {
    if (auto* anim = entity.get_component<Components::Animation>()) {
      anim->startTime = Engine::Time::get_time();
    }
    if (auto* atlas = entity.get_component<Components::TextureAtlas>()) {
      atlas->drawIndex = 0;
    }
  }

  inline bool has_animation_finished(Engine::ECS::Entity entity) {
    auto* anim = entity.get_component<Components::Animation>();
    if (!anim) return true;

    if (!anim->is_looping) return anim->has_ended();

    const float dur = anim->totalDuration;
    if (dur <= 0.0f) return false;

    const float elapsed = Engine::Time::get_time() - anim->startTime;
    return elapsed >= dur;
  }

  // Healthbar helpers
  inline void ensure_wave_bars_component(Engine::ECS::Entity boss_entity) {
    if (boss_entity.get_component<Components::CatMonkeySummonWaveHealthBars>()) return;
    boss_entity.add_component<Components::CatMonkeySummonWaveHealthBars>();
  }

  inline void mark_wave_cleared(Engine::ECS::Entity boss_entity, int wave_index) {
    if (wave_index < 0 || wave_index >= k_summon_state_count) return;

    BossRuntime& rt = runtime_for(boss_entity);
    if (!rt.wave_spawned[static_cast<size_t>(wave_index)] || rt.wave_cleared[static_cast<size_t>(wave_index)]) return;

    rt.wave_cleared[static_cast<size_t>(wave_index)] = true;
    rt.wave_summons[static_cast<size_t>(wave_index)].clear();

    if (rt.active_wave_index == wave_index) {
      rt.active_wave_index = -1;
    }

    const float now = Engine::Time::get_time();

    // Add a short breather after a wave is cleared before the next summon can begin.
    rt.next_summon_allowed_time = std::max(rt.next_summon_allowed_time, now + 2.0f);

    // Summon5: we may have temporarily retargeted the Centered camera's Parent to the boss.
    // Restore it when the wave is fully cleared.
    if (wave_index == 4) {
      restore_centered_camera_parent(rt);
    }

    ensure_wave_bars_component(boss_entity);
    if (auto* bars = boss_entity.get_component<Components::CatMonkeySummonWaveHealthBars>()) {
      switch (wave_index) {
        case 0: bars->wave1_cleared = true; bars->wave1_cleared_time = now; break;
        case 1: bars->wave2_cleared = true; bars->wave2_cleared_time = now; break;
        case 2: bars->wave3_cleared = true; bars->wave3_cleared_time = now; break;
        case 3: bars->wave4_cleared = true; bars->wave4_cleared_time = now; break;
        case 4: bars->wave5_cleared = true; bars->wave5_cleared_time = now; break;
      }
    }
  }

  inline void update_wave_healthbars_component(Engine::ECS::Entity boss_entity) {
    ensure_wave_bars_component(boss_entity);
    auto* bars = boss_entity.get_component<Components::CatMonkeySummonWaveHealthBars>();
    if (!bars) return;

    if (auto* boss_hp = boss_entity.get_component<Components::EnemyHealth>()) {
      if (boss_hp->current_hp <= 0) {
        bars->visible = false;
        return;
      }
    }
    bars->visible = true;

    BossRuntime& rt = runtime_for(boss_entity);

    auto set_wave = [&](int idx, int& cur, int& mx, bool& spawned, bool& cleared) {
      spawned = rt.wave_spawned[idx];
      cleared = rt.wave_cleared[idx];
      mx = std::max(0, rt.wave_pool_max_hp[idx]);
      if (!spawned) {
        cur = mx = 0;
        return;
      }
      if (cleared) {
        cur = 0;
        return;
      }

      auto& list = rt.wave_summons[idx];
      // Prune dead/inactive summons so waves can clear.
      list.erase(
        std::remove_if(list.begin(), list.end(), [](Engine::ECS::EntityID id) {
          Engine::ECS::Entity e(id);
          if (!Engine::ECS::is_entity_active(e)) return true;
          if (auto* eh = e.get_component<Components::EnemyHealth>()) {
            return eh->current_hp <= 0;
          }
          return false;
        }),
        list.end());

      cur = [&]() {
        int total = 0;
        for (Engine::ECS::EntityID id : list) {
          Engine::ECS::Entity e(id);
          if (!Engine::ECS::is_entity_active(e)) continue;
          if (auto* eh = e.get_component<Components::EnemyHealth>()) total += std::max(0, eh->current_hp);
        }
        // For Summon5, include not-yet-spawned enemies as full HP so the wave bar
        // reflects all entities listed in the wave's spawn JSON.
        if (idx == 4) {
          total += std::max(0, rt.summon5_pending_hp_remaining);
        }
        return total;
      }();

      if (list.empty()) {
        if (idx == 4 && summon5_has_more_to_spawn(rt)) return;
        mark_wave_cleared(boss_entity, idx);
      }
    };

    set_wave(0, bars->wave1_current_hp, bars->wave1_max_hp, bars->wave1_spawned, bars->wave1_cleared);
    set_wave(1, bars->wave2_current_hp, bars->wave2_max_hp, bars->wave2_spawned, bars->wave2_cleared);
    set_wave(2, bars->wave3_current_hp, bars->wave3_max_hp, bars->wave3_spawned, bars->wave3_cleared);
    set_wave(3, bars->wave4_current_hp, bars->wave4_max_hp, bars->wave4_spawned, bars->wave4_cleared);
    set_wave(4, bars->wave5_current_hp, bars->wave5_max_hp, bars->wave5_spawned, bars->wave5_cleared);

    // Also drive the combined summon healthbar (single bar for the active wave).
    if (auto* bar = boss_entity.get_component<Components::CatMonkeySummonHealthBar>()) {
      const int idx = rt.active_wave_index;
      if (idx < 0 || idx >= k_summon_state_count || !rt.wave_spawned[idx] || rt.wave_cleared[idx]) {
        bar->visible = false;
        bar->total_current_hp = 0;
        bar->total_max_hp = 0;
      } else {
        bar->visible = true;
        bar->total_max_hp = std::max(0, rt.wave_pool_max_hp[idx]);
        switch (idx) {
          case 0: bar->total_current_hp = std::max(0, bars->wave1_current_hp); break;
          case 1: bar->total_current_hp = std::max(0, bars->wave2_current_hp); break;
          case 2: bar->total_current_hp = std::max(0, bars->wave3_current_hp); break;
          case 3: bar->total_current_hp = std::max(0, bars->wave4_current_hp); break;
          case 4: bar->total_current_hp = std::max(0, bars->wave5_current_hp); break;
          default: bar->total_current_hp = 0; break;
        }
      }
    }
  }

  // Returns true if all waves are cleared
  bool all_waves_cleared(const BossRuntime& rt) {
    for (int i = 0; i < k_summon_state_count; ++i) {
      if (!rt.wave_cleared[i]) return false;
    }
    return true;
  }

  // Null callbacks
  void null_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    if (entity.is_valid()) {
      BossRuntime& rt = runtime_for(entity);
      rt.camera_override_active = false;
      rt.camera_override_until = 0.0f;
      rt.centered_cut_override_zoom = false;
      restore_centered_camera_parent(rt);
    }
    activate_player_camera_if_present();
  }

  void null_on_update([[maybe_unused]] Engine::ECS::Entity entity) {}

  // Idle callbacks
  void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    if (is_ai_disabled(entity)) {
      if (auto* sm = entity.get_component<Components::StateMachine>()) {
        sm->change_state("Null");
      }
      return;
    }

    BossRuntime& rt = runtime_for(entity);
    ensure_city_layers_hidden(rt);

    // Cache the scene-load Centered camera baseline so Summon1-4 can always reset to it.
    ensure_centered_camera_baseline(rt);

    if (!rt.summon_pacing_initialized) {
      rt.summon_pacing_initialized = true;
      rt.next_summon_allowed_time = Engine::Time::get_time() + summon_cooldown(entity);
    }

    // Enable ambient camera shake on first idle enter.
    if (!rt.ambient_shake_active) {
      rt.ambient_shake_active = true;
      static std::mt19937 rng{ std::random_device{}() };
      std::uniform_real_distribution<float> dist(4.0f, 10.0f);
      rt.ambient_shake_next_time = Engine::Time::get_time() + dist(rng);
      rt.ambient_shake_tick = 0;
      rt.ambient_shake_offset_y = 0.0f;
    }
  }

  void idle_on_update(Engine::ECS::Entity entity) {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();
    if (!sm || !hp || hp->current_hp <= 0 || is_ai_disabled(entity)) {
      if (sm) sm->change_state("Null");
      return;
    }

    BossRuntime& rt = runtime_for(entity);
    tick_city_scroll_if_enabled(rt);
    tick_mid_building_if_active(rt);
    tick_ambient_camera_shake(rt);
    tick_tunnel_if_active(rt);
    tick_train_tracks_if_active(rt);
    // Summon5 timed spawning: continue spawning queued minions over time.
    if (rt.active_wave_index == 4 && rt.summon5_timed_spawning) {
      const float now_spawn = Engine::Time::get_time();
      if (now_spawn >= rt.summon5_next_spawn_time && rt.summon5_next_index < rt.summon5_pending_prefabs.size()) {
        const int mhp = (rt.summon5_next_index < rt.summon5_pending_max_hp.size())
          ? rt.summon5_pending_max_hp[rt.summon5_next_index]
          : 0;
        Engine::ECS::Entity spawned = spawn_enemy_from_prefab(
          rt.summon5_pending_prefabs[rt.summon5_next_index],
          summon5_spawn_pos_at(rt, rt.summon5_next_index));

        // This spawn slot is now "consumed" regardless of success.
        rt.summon5_pending_hp_remaining = std::max(0, rt.summon5_pending_hp_remaining - std::max(0, mhp));

        if (spawned.is_valid()) {
          rt.wave_summons[4].push_back(spawned.get_id());
        }

        ++rt.summon5_next_index;
        rt.summon5_next_spawn_time = now_spawn + std::max(0.01f, rt.summon5_current_spawn_interval_sec);

        if (rt.summon5_next_index >= rt.summon5_pending_prefabs.size()) {
          rt.summon5_timed_spawning = false;
        }
      }
    }

    const float now = Engine::Time::get_time();
    if (!rt.summon_pacing_initialized) {
      rt.summon_pacing_initialized = true;
      rt.next_summon_allowed_time = now + summon_cooldown(entity);
    }

    // Camera override handling
    if (rt.camera_override_active) {
      if (now < rt.camera_override_until) {
        if (rt.camera_override_mode == BossRuntime::CameraOverrideMode::Centered) {
          activate_centered_camera_if_present();
          tick_post_spawn_cinematic_camera(rt);
        } else {
          activate_cat_monkey_camera_if_present();
        }
      } else {
        // Cut back from CatMonkey -> Centered.
        ensure_centered_camera_baseline(rt);
        apply_centered_camera_baseline(rt);

        // Summon5: avoid an abrupt zoom jump when switching cameras by overriding
        // the Centered camera's zoom to match the Cat_Monkey camera at the cut.
        if (rt.centered_cut_override_zoom) {
          Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
          if (centered.is_valid()) {
            if (auto* t = centered.get_component<Components::Transform2D>()) {
              const float z = std::clamp(rt.centered_cut_zoom, 0.1f, 10.0f);
              t->scale = { z, z };
              t->position = rt.post_spawn_pos_start;
              t->rotation = rt.post_spawn_rot_start;
            }
          }
          rt.centered_cut_override_zoom = false; // one-shot
        }

        rt.camera_override_active = true;
        rt.camera_override_mode = BossRuntime::CameraOverrideMode::Centered;
        rt.camera_override_until = std::numeric_limits<float>::infinity();
        activate_centered_camera_if_present();
      }
    }

    // Wave management and next summon
    // [Simplified - full logic in original, extract process_tracked_minion_deaths etc]
    if (all_waves_cleared(rt))
    {
      if (!rt.win_cutscene_timer_started)
      {
        rt.win_cutscene_timer_started = true;
        rt.win_cutscene_trigger_time = now + 5.0f;

        // Smoothly zoom the Centered camera back to its scene-load baseline over 2.5s.
        restore_centered_camera_parent(rt);
        ensure_centered_camera_baseline(rt);

        // Sample current camera state as the start of the lerp.
        Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
        if (centered.is_valid()) {
          if (auto* t = centered.get_component<Components::Transform2D>()) {
            rt.post_spawn_pos_start  = t->position;
            rt.post_spawn_rot_start  = t->rotation;
            rt.post_spawn_zoom_start = t->scale.x;
          }
        }

        // Baseline is the target.
        rt.post_spawn_pos_target  = rt.centered_cam_baseline_pos;
        rt.post_spawn_rot_target  = rt.centered_cam_baseline_rot;
        rt.post_spawn_zoom_target = rt.centered_cam_baseline_zoom;

        rt.post_spawn_cinematic_start_time = now;
        rt.post_spawn_cinematic_end_time   = now + 2.5f;
        rt.post_spawn_cinematic_active     = true;
        rt.pending_summon_immediate_zoom   = false; // ensure cinematic path runs

        // Keep camera override on Centered so tick_post_spawn_cinematic_camera fires.
        rt.camera_override_active = true;
        rt.camera_override_mode   = BossRuntime::CameraOverrideMode::Centered;
        rt.camera_override_until  = std::numeric_limits<float>::infinity();
        activate_centered_camera_if_present();
      }
      else if (now >= rt.win_cutscene_trigger_time)
      {
        Engine::SceneManager::load_scene("EndCutsceneState");
      }
      return;
    }

    bool cooldown_ready = now >= rt.next_summon_allowed_time;
    if (rt.active_wave_index == -1 && cooldown_ready) {
      const int idx = rt.next_summon_index % k_summon_state_count;
      const char* state = idx == 1 ? "Summon2" : idx == 2 ? "Summon3" : idx == 3 ? "Summon4" : idx == 4 ? "Summon5" : "Summon1";
      sm->change_state(state);
    }
    update_wave_healthbars_component(entity);
  }

  // Summon impl
  void summon_impl_on_enter(Engine::ECS::Entity entity, int summon_index) {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();
    if (!sm || !hp || hp->current_hp <= 0 || is_ai_disabled(entity)) {
      if (sm) sm->change_state("Null");
      return;
    }

    BossRuntime& rt = runtime_for(entity);
    rt.last_summon_time = Engine::Time::get_time();
    rt.next_summon_index = (rt.next_summon_index + 1) % k_summon_state_count;

    rt.summon_pending = true;
    rt.pending_summon_index = summon_index;
    rt.pending_summon_immediate_zoom = (summon_index < 4);
    if (summon_index >= 0 && summon_index < k_summon_state_count) {
      rt.wave_summons[summon_index].clear();
      rt.wave_pool_max_hp[summon_index] = 0;
    }

    restart_entity_animation(entity);
    update_wave_healthbars_component(entity);

    rt.camera_override_active = true;
    rt.camera_override_until = std::numeric_limits<float>::infinity();
    rt.camera_override_mode = BossRuntime::CameraOverrideMode::CatMonkey;
    rt.post_spawn_cinematic_active = false;
    activate_cat_monkey_camera_if_present();
  }

  void summon_impl_on_update(Engine::ECS::Entity entity) {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();
    if (!sm || !hp || hp->current_hp <= 0 || is_ai_disabled(entity)) {
      if (sm) sm->change_state("Null");
      return;
    }

    BossRuntime& rt = runtime_for(entity);

    tick_city_scroll_if_enabled(rt);
    tick_mid_building_if_active(rt);
    tick_ambient_camera_shake(rt);
    tick_tunnel_if_active(rt);
    tick_train_tracks_if_active(rt);

    if (!has_animation_finished(entity)) {
      // Summon5 no longer uses the Cat_Monkey shake; skip that tick.
      return;
    }

    // Stop shake
    if (rt.pending_summon_index == 4) {
      rt.summon5_shake_active = false;
      Engine::ECS::Entity cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
      if (cam.is_valid()) {
        if (auto* t = cam.get_component<Components::Transform2D>()) {
          t->position = rt.summon5_cat_cam_base_pos;
          t->rotation = rt.summon5_cat_cam_base_rot;
        }
      }
    }

    spawn_wave_for_summon_index(entity, rt.pending_summon_index);

    // Summon5: ensure Centered camera behaves like Summon1-4 (fixed, no drift).
    if (rt.pending_summon_index == 4) {
      detach_centered_camera_parent_for_cinematic(rt);
    }

    rt.summon_pending = false;
    const bool immediate_zoom = rt.pending_summon_immediate_zoom;
    rt.pending_summon_immediate_zoom = false;
    const bool is_summon5 = (rt.pending_summon_index == 4);

    const float now = Engine::Time::get_time();
    rt.next_summon_allowed_time = now + summon_cooldown(entity);

    if (is_summon5) {
      // Already on Centered camera — no linger needed, start cinematic immediately.
      rt.camera_override_active = true;
      rt.camera_override_mode   = BossRuntime::CameraOverrideMode::Centered;
      rt.camera_override_until  = std::numeric_limits<float>::infinity();
      activate_centered_camera_if_present();
      rt.post_spawn_cinematic_start_time = now;
    } else {
      // Linger on CatMonkey, then cut to Centered.
      rt.camera_override_active = true;
      rt.camera_override_mode   = BossRuntime::CameraOverrideMode::CatMonkey;
      rt.camera_override_until  = now + k_post_summon_camera_linger_sec;
      activate_cat_monkey_camera_if_present();
      rt.post_spawn_cinematic_start_time = now + k_post_summon_camera_linger_sec;
    }

    // Prepare the Centered camera cinematic.
    // For Summon5 the camera is already placed (done in summon5_on_enter), so
    // skip the baseline reset that would clobber it.
    Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
    if (!is_summon5) {
      ensure_centered_camera_baseline(rt);
      // Always reset the Centered camera to the scene-load view before we start the post-summon pan-out.
      // This keeps Summon1-4 consistent across waves.
      apply_centered_camera_baseline(rt);
      if (centered.is_valid()) {
        if (auto* t = centered.get_component<Components::Transform2D>()) {
          rt.post_spawn_pos_start = rt.centered_cam_baseline_saved ? rt.centered_cam_baseline_pos : t->position;
          rt.post_spawn_pos_target = rt.post_spawn_pos_start;
          rt.post_spawn_rot_start = rt.centered_cam_baseline_saved ? rt.centered_cam_baseline_rot : t->rotation;
          rt.post_spawn_rot_target = rt.post_spawn_rot_start;
          rt.post_spawn_zoom_start = rt.centered_cam_baseline_saved ? rt.centered_cam_baseline_zoom : t->scale.x;
        }
      }
    }

    rt.post_spawn_cinematic_active = true;

    if (immediate_zoom) {
      // Summon1-4: small smooth pan-out (subtle zoom-out) to reduce harshness.
      rt.post_spawn_cinematic_end_time = rt.post_spawn_cinematic_start_time + 0.75f;
      rt.post_spawn_zoom_target = std::clamp(rt.post_spawn_zoom_start * 0.90f, 0.1f, 10.0f);
    } else {
      // Summon5: Centered camera is already at k_summon5_zoom_in_start at the
      // boss position (set in summon5_on_enter). Just configure the pan-out target.
      if (centered.is_valid()) {
        if (auto* t = centered.get_component<Components::Transform2D>()) {
          rt.post_spawn_pos_start  = t->position;
          rt.post_spawn_rot_start  = t->rotation;
          rt.post_spawn_rot_target = t->rotation;
        }
      }
      rt.post_spawn_zoom_start  = k_summon5_zoom_in_start;
      rt.post_spawn_pos_target  = k_summon5_target_pos;
      rt.centered_cut_override_zoom = false; // no cut needed, already on Centered

      rt.post_spawn_cinematic_end_time = rt.post_spawn_cinematic_start_time + k_summon5_cinematic_duration_sec;
      rt.post_spawn_zoom_target = k_summon5_target_zoom;
    }

    sm->change_state("Idle");
  }

  // Individual summon wrappers
  void summon1_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 0); }
  void summon1_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  void summon2_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 1); }
  void summon2_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  void summon3_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 2); }
  void summon3_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  void summon4_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 3); }
  void summon4_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  void summon5_on_enter(Engine::ECS::Entity entity) {
    summon_impl_on_enter(entity, 4);
    BossRuntime& rt = runtime_for(entity);
    rt.tunnel_destroyed = true;
    destroy_tunnel(rt);
    rt.city_scroll_enabled = true;
    rt.mid_building_enabled = true;

    // Summon5: skip Cat_Monkey camera entirely. Immediately place the Centered
    // camera at 4.5 zoom at the Cat_Monkey camera's current world position,
    // then switch to it so the cinematic pan-out starts from there.
    Engine::ECS::Entity cat_cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
    Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);

    Math::Vec2D start_pos = {};
    float start_rot = 0.0f;
    if (cat_cam.is_valid()) {
      if (auto* ct = cat_cam.get_component<Components::Transform2D>()) {
        start_pos = ct->position;
        start_rot  = ct->rotation;
      }
    }

    if (centered.is_valid()) {
      if (auto* t = centered.get_component<Components::Transform2D>()) {
        t->position = start_pos;
        t->rotation = start_rot;
        t->scale    = { k_summon5_zoom_in_start, k_summon5_zoom_in_start };
      }
    }

    // Override the CatMonkey camera mode set by summon_impl_on_enter.
    rt.camera_override_mode = BossRuntime::CameraOverrideMode::Centered;
    activate_centered_camera_if_present();
  }
  void summon5_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }

  void city_layer_active_on_enter(Engine::ECS::Entity entity)
  {
    if (auto* color = entity.get_component<Components::Color>()) {
      color->a = 1.0f;
    }

    if (auto* sm = entity.get_component<Components::StateMachine>()) {
      // If a timeline clip exists on this state, start it when the layer becomes active.
      sm->play_property_anim();
    }
  }

  void city_layer_active_on_update(Engine::ECS::Entity entity)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    if (!sm) {
      return;
    }

    auto it = sm->states.find(sm->get_current_state());
    if (it == sm->states.end() || !it->second.property_animation.has_value()) {
      return;
    }

    // Keep looping clips alive without requiring manual replay.
    if (it->second.property_animation->is_looping && !sm->is_property_anim_playing()) {
      sm->play_property_anim();
    }
  }

  void summon_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 0); }
  void summon_on_update([[maybe_unused]] Engine::ECS::Entity entity) {}

  // Hit/Death
  void hit_on_enter(Engine::ECS::Entity entity) {
    if (auto* sm = entity.get_component<Components::StateMachine>()) {
      sm->change_state("Idle");
    }
  }

  void death_on_enter(Engine::ECS::Entity entity) {
    // Ensure we don't leave the Centered camera parented/retargeted.
    if (entity.is_valid()) {
      BossRuntime& rt = runtime_for(entity);
      restore_centered_camera_parent(rt);
    }
    if (auto* bar = entity.get_component<Components::CatMonkeySummonHealthBar>()) {
      bar->visible = false;
    }
    if (auto* bars = entity.get_component<Components::CatMonkeySummonWaveHealthBars>()) {
      bars->visible = false;
    }
    activate_player_camera_if_present();
  }

  void death_on_update(Engine::ECS::Entity entity) {
    if (entity.is_valid()) {
      BossRuntime& rt = runtime_for(entity);
      tick_city_scroll_if_enabled(rt);
      tick_mid_building_if_active(rt);
      tick_ambient_camera_shake(rt);
      tick_tunnel_if_active(rt);
      tick_train_tracks_if_active(rt);
    }

    if (auto* anim = entity.get_component<Components::Animation>()) {
      if (!anim->has_ended()) return;
    }

    clear_runtime_for(entity);
    Engine::ECS::destroy_entity(entity);
  }

  void register_catmonkey_blueprint_runtime() {
    auto& registry = ::Engine::StateMachineBlueprintRegistry::instance();

    ::Engine::StateMachineBlueprint catmonkey_bp;
    catmonkey_bp.initial_state = "Idle";
    {
      auto& bp = catmonkey_bp;
      STATE("Null", CatMonkeyStateCallbacks::null_on_enter, CatMonkeyStateCallbacks::null_on_update, NO_CALLBACK)
      STATE("Idle", CatMonkeyStateCallbacks::idle_on_enter, CatMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)

      STATE("Summon1", CatMonkeyStateCallbacks::summon1_on_enter, CatMonkeyStateCallbacks::summon1_on_update, NO_CALLBACK)
      STATE("Summon2", CatMonkeyStateCallbacks::summon2_on_enter, CatMonkeyStateCallbacks::summon2_on_update, NO_CALLBACK)
      STATE("Summon3", CatMonkeyStateCallbacks::summon3_on_enter, CatMonkeyStateCallbacks::summon3_on_update, NO_CALLBACK)
      STATE("Summon4", CatMonkeyStateCallbacks::summon4_on_enter, CatMonkeyStateCallbacks::summon4_on_update, NO_CALLBACK)
      STATE("Summon5", CatMonkeyStateCallbacks::summon5_on_enter, CatMonkeyStateCallbacks::summon5_on_update, NO_CALLBACK)

      STATE("Hit", CatMonkeyStateCallbacks::hit_on_enter, NO_CALLBACK, NO_CALLBACK)
      STATE("Death", CatMonkeyStateCallbacks::death_on_enter, CatMonkeyStateCallbacks::death_on_update, NO_CALLBACK)
    }

    registry.register_blueprint("CatMonkey", catmonkey_bp);

    ::Engine::StateMachineBlueprint city_bp;
    city_bp.initial_state = "Active";
    {
      auto& bp = city_bp;
      STATE("Active", CatMonkeyStateCallbacks::city_layer_active_on_enter, CatMonkeyStateCallbacks::city_layer_active_on_update, NO_CALLBACK)
    }

    registry.register_blueprint("CatMonkeyCityLayer", city_bp);
  }
}

// Backwards-compat aliases
namespace {
  void Cat_monkey_idle_enter(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::idle_on_enter(e); }
  void Cat_monkey_idle_update(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::idle_on_update(e); }
  void Cat_monkey_null_enter(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::null_on_enter(e); }
  void Cat_monkey_null_update(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::null_on_update(e); }
  void Cat_monkey_summon_enter(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::summon_on_enter(e); }
  void Cat_monkey_summon_update(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::summon_on_update(e); }
  void Cat_monkey_summon1_enter(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::summon1_on_enter(e); }
  void Cat_monkey_summon1_update(Engine::ECS::Entity e) { CatMonkeyStateCallbacks::summon1_on_update(e); }
  // ... (similar for summon2-5, hit, death)

  REGISTER_FUNCTION(Cat_monkey_idle_enter)
  REGISTER_FUNCTION(Cat_monkey_idle_update)
  // ... all others
}





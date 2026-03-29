//-----------------------------------------------------------------------------------
// file:    CatMonkeyBossRuntime.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Runtime state and helpers for Cat Monkey boss fight.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include <unordered_map>
#include <array>
#include <vector>
#include <string>
#include <filesystem>

namespace CatMonkeyStateCallbacks
{
  namespace 
  {
    static constexpr int k_summon_state_count = 5;
    static constexpr const char* k_cat_monkey_camera_name = "Cat_Monkey";
    static constexpr const char* k_centered_camera_name = "Centered";

    static constexpr float k_post_summon_camera_linger_sec = 1.0f;
    static constexpr float k_summon5_cinematic_duration_sec = 6.0f;
    static constexpr float k_summon5_target_zoom = 0.65f;
    static constexpr int k_summon5_shake_count = 8;
    static constexpr float k_summon5_shake_interval_sec = 0.05f;
    static constexpr float k_summon5_shake_amplitude = 12.0f;
  }

  struct BossRuntime
  {
    float last_summon_time = -9999.0f;
    std::array<std::vector<Engine::ECS::EntityID>, k_summon_state_count> wave_summons{};
    std::array<int, k_summon_state_count> wave_pool_max_hp{};
    std::array<bool, k_summon_state_count> wave_spawned{};
    std::array<bool, k_summon_state_count> wave_cleared{};

    int next_summon_index = 0;
    bool summon_pending = false;
    int pending_summon_index = 0;
    bool pending_summon_immediate_zoom = false;
    int active_wave_index = -1;

    // Tunnel entity for summon 1-4
    Engine::ECS::EntityID tunnel_entity_id = Engine::ECS::INVALID_ENTITY;
    bool tunnel_destroyed = false;

    // Train tracks entity loops throughout the whole duration
    Engine::ECS::EntityID train_tracks_entity_id = Engine::ECS::INVALID_ENTITY;
    bool train_tracks_destroyed = false;

    // Special handling for Summon5: spawn from the ceiling slowly.
    bool summon5_timed_spawning = false;
    std::vector<std::string> summon5_pending_prefabs{};
    std::vector<Math::Vec2D> summon5_spawn_positions{};
    std::vector<int> summon5_pending_max_hp{};
    int summon5_pending_hp_remaining = 0;
    size_t summon5_next_index = 0;
    float summon5_next_spawn_time = 0.0f;

    // Summon5 spawn rate ramping: starts slow and speeds up on kills.
    float summon5_current_spawn_interval_sec = 5.0f;

    // Summon pacing: enforce a chill period before the first summon and
    // between successive summons.
    bool summon_pacing_initialized = false;
    float next_summon_allowed_time = 0.0f;

    // When active, we may temporarily retarget the Centered camera's Parent to the boss
    // to avoid drifting while we zoom/pan.
    bool centered_cam_parent_saved = false;
    bool centered_cam_had_parent = false;
    bool centered_cam_parent_added = false;
    Components::Parent centered_cam_saved_parent{};

    // Centered camera baseline (captured once from the level on first use).
    // Used so Summon1-4 can always reset to the scene-load Centered camera view
    // before doing the small pan-out.
    bool centered_cam_baseline_saved = false;
    Math::Vec2D centered_cam_baseline_pos{};
    float centered_cam_baseline_rot = 0.0f;
    float centered_cam_baseline_zoom = 1.0f;

    // Camera override during summoning.
    bool camera_override_active = false;
    float camera_override_until = 0.0f;

    enum class CameraOverrideMode : uint8_t
    {
      CatMonkey,
      Centered
    };
    CameraOverrideMode camera_override_mode = CameraOverrideMode::CatMonkey;

    // Post-spawn cinematic zoom (drives Centered camera Transform2D.scale.x).
    bool post_spawn_cinematic_active = false;
    float post_spawn_cinematic_start_time = 0.0f;
    float post_spawn_cinematic_end_time = 0.0f;
    float post_spawn_zoom_start = 1.0f;
    float post_spawn_zoom_target = 1.0f;
    Math::Vec2D post_spawn_pos_start{};
    Math::Vec2D post_spawn_pos_target{};
    float post_spawn_rot_start = 0.0f;
    float post_spawn_rot_target = 0.0f;

    // Summon5: begin a slow pan-out on the Cat_Monkey camera during the linger,
    // then cut to Centered and continue the pan-out without a zoom jump.
    bool summon5_cat_post_zoom_active = false;
    float summon5_cat_post_zoom_start_time = 0.0f;
    float summon5_cat_post_zoom_end_time = 0.0f;
    float summon5_cat_post_zoom_start = 1.0f;
    float summon5_cat_post_zoom_target = 1.0f;

    bool centered_cut_override_zoom = false;
    float centered_cut_zoom = 1.0f;

    // Summon5: quick shake on the Cat_Monkey camera during summon animation.
    bool summon5_shake_active = false;
    int summon5_shakes_done = 0;
    float summon5_next_shake_time = 0.0f;
    Math::Vec2D summon5_cat_cam_base_pos{};
    float summon5_cat_cam_base_rot = 0.0f;

    // City background scroll (enabled when Summon5 is reached).
    bool city_layers_hidden = false;
    bool city_scroll_enabled = false;
    bool city_scroll_initialized = false;

    // Debug (one-time logs; avoids per-frame spam).
    bool debug_logged_summon5_enter = false;
    bool debug_logged_city_init = false;
    bool debug_logged_centered_camera_missing = false;
  };

  extern std::unordered_map<Engine::ECS::EntityID, BossRuntime>& runtimes();
  BossRuntime& runtime_for(Engine::ECS::Entity boss);
  void clear_runtime_for(Engine::ECS::Entity boss);

  // Camera helpers
  Engine::ECS::Entity find_camera_entity_by_name(const char* name);
  void set_active_camera_entity(Engine::ECS::Entity camera_entity);
  void activate_player_camera_if_present();
  void activate_cat_monkey_camera_if_present();
  void activate_centered_camera_if_present();

  // City scroll system
  void ensure_city_layers_hidden(BossRuntime& rt);
  void tick_city_scroll_if_enabled(BossRuntime& rt);

  // Shake system
  void start_summon5_cat_camera_shake(BossRuntime& rt);
  void tick_summon5_cat_camera_shake(BossRuntime& rt);

  // Cinematic system
  void tick_post_spawn_cinematic_camera(BossRuntime& rt);

  // Camera parenting helpers (used to keep Centered camera from drifting during cinematics)
  void detach_centered_camera_parent_for_cinematic(BossRuntime& rt);
  void restore_centered_camera_parent(BossRuntime& rt);

  // Util
  Engine::ECS::Entity find_entity_by_name_with_transform(const char* name);
  void set_entity_alpha_if_present(Engine::ECS::Entity entity, float alpha);
  void detach_from_parent_if_present(Engine::ECS::Entity entity);
}


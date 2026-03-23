//-----------------------------------------------------------------------------------
// file:    CatMonkeyStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Cat Monkey boss state machine callback implementations (serializable)
//-----------------------------------------------------------------------------------

#include "pch.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <random>

#include "../Components/EnemyHealthComponent.h"
#include "../Components/CatMonkeyBossComponent.h"
#include "../Components/CatMonkeySummonHealthBarComponent.h"
#include "../Components/CatMonkeySummonWaveHealthBarsComponent.h"
#include "../../Engine/Reflection/ReflectionLibrary.h"
#include "../../Engine/Graphics/Renderer.h"

namespace CatMonkeyStateCallbacks
{
  namespace
  {
    static constexpr int k_summon_state_count = 5;

    // Camera entity names used in LevelTwo.
    static constexpr const char* k_player_camera_name = "Player_Camera";
    static constexpr const char* k_cat_monkey_camera_name = "Cat_Monkey";
    static constexpr const char* k_centered_camera_name = "Centered";

    // Background layer entity names used in LevelTwo.
    static constexpr const char* k_cityscape_layer_name = "cityscape";
    static constexpr const char* k_minicity_layer_name = "minicity";
    static constexpr const char* k_foreground_city_layer_name = "foreground_city";
    static constexpr const char* k_background_city_layer_name = "background_city";

    // Scroll speeds (world units / sec). Tuned for LevelTwo scale.
    static constexpr float k_cityscape_speed = 200.0f;
    static constexpr float k_minicity_speed = 200.0f;
    static constexpr float k_foreground_city_speed = 180.0f;   // Slightly slower than cityscape
    static constexpr float k_background_city_speed = 350.0f;   // Blows past faster than everything else

    // How long to keep the Cat_Monkey camera active after the summon animation
    // completes and enemies are spawned, so the player can see the wave appear.
    static constexpr float k_post_summon_camera_linger_sec = 1.0f;

    // Summon5 cinematic: cut to Centered camera and zoom out to show the
    // scrolling city layers (train background effect).
    static constexpr float k_summon5_cinematic_duration_sec = 3.5f;
    static constexpr float k_summon5_target_zoom = 0.65f; // smaller = more zoomed out

    inline Engine::ECS::Entity spawn_enemy_from_prefab(const std::string& prefab_path, const Math::Vec2D& pos);
    inline Engine::ECS::Entity find_camera_entity_by_name(const char* name);

    inline bool get_tilemap_world_bounds(Math::Vec2D& min, Math::Vec2D& max)
    {
      min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
      max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
      bool found_any = false;

      for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
        auto* tilemap = entity.get_component<Components::TileMap>();
        if (!tilemap || tilemap->is_empty()) {
          continue;
        }

        int grid_min_x, grid_min_y, grid_max_x, grid_max_y;
        tilemap->get_bounds(grid_min_x, grid_min_y, grid_max_x, grid_max_y);

        const float min_world_x = grid_min_x * tilemap->tile_size.x + tilemap->grid_offset.x;
        const float min_world_y = grid_min_y * tilemap->tile_size.y + tilemap->grid_offset.y;
        const float max_world_x = (grid_max_x + 1) * tilemap->tile_size.x + tilemap->grid_offset.x;
        const float max_world_y = (grid_max_y + 1) * tilemap->tile_size.y + tilemap->grid_offset.y;

        if (min_world_x < min.x) min.x = min_world_x;
        if (min_world_y < min.y) min.y = min_world_y;
        if (max_world_x > max.x) max.x = max_world_x;
        if (max_world_y > max.y) max.y = max_world_y;

        found_any = true;
      }

      return found_any;
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
      int active_wave_index = -1;

      // Special handling for Summon5: spawn from the ceiling slowly.
      bool summon5_timed_spawning = false;
      std::vector<std::string> summon5_pending_prefabs{};
      std::vector<Math::Vec2D> summon5_spawn_positions{};
      size_t summon5_next_index = 0;
      float summon5_next_spawn_time = 0.0f;

      // Summon5 spawn rate ramping: starts slow and speeds up on kills.
      float summon5_current_spawn_interval_sec = 5.0f;

      // Summon pacing: enforce a chill period before the first summon and
      // between successive summons.
      bool summon_pacing_initialized = false;
      float next_summon_allowed_time = 0.0f;

      // Camera override during summoning.
      bool camera_override_active = false;
      float camera_override_until = 0.0f;

      enum class CameraOverrideMode : uint8_t
      {
        CatMonkey,
        Centered
      };
      CameraOverrideMode camera_override_mode = CameraOverrideMode::CatMonkey;

      // Summon5 cinematic zoom (drives Centered camera Transform2D.scale.x).
      bool summon5_cinematic_active = false;
      float summon5_cinematic_start_time = 0.0f;
      float summon5_cinematic_end_time = 0.0f;
      float summon5_zoom_start = 1.0f;
      Math::Vec2D summon5_pos_start{};
      Math::Vec2D summon5_pos_target{};
      float summon5_rot_start = 0.0f;
      float summon5_rot_target = 0.0f;

      // City background scroll (enabled when Summon5 is reached).
      bool city_layers_hidden = false;
      bool city_scroll_enabled = false;
      bool city_scroll_initialized = false;
      float cityscape_start_x = 0.0f;
      float minicity_start_x = 0.0f;
      float foreground_city_start_x = 0.0f;
      float background_city_start_x = 0.0f;

      // Debug (one-time logs; avoids per-frame spam).
      bool debug_logged_summon5_enter = false;
      bool debug_logged_city_init = false;
      bool debug_logged_centered_camera_missing = false;
      bool debug_logged_tilemap_bounds_missing = false;
    };

    inline Engine::ECS::Entity find_entity_by_name_with_transform(const char* name)
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

    inline void set_entity_alpha_if_present(Engine::ECS::Entity entity, float alpha)
    {
      if (auto* color = entity.get_component<Components::Color>()) {
        color->a = std::clamp(alpha, 0.0f, 1.0f);
      }
    }

    inline void detach_from_parent_if_present(Engine::ECS::Entity entity)
    {
      if (entity.is_valid() && entity.has_component<Components::Parent>()) {
        entity.remove_component<Components::Parent>();
      }
    }

    inline void ensure_city_layers_hidden(BossRuntime& rt)
    {
      if (rt.city_layers_hidden) {
        return;
      }

      // Hide the city layers until Summon5 is reached.
      set_entity_alpha_if_present(find_entity_by_name_with_transform(k_cityscape_layer_name), 0.0f);
      set_entity_alpha_if_present(find_entity_by_name_with_transform(k_minicity_layer_name), 0.0f);
      set_entity_alpha_if_present(find_entity_by_name_with_transform(k_foreground_city_layer_name), 0.0f);
      set_entity_alpha_if_present(find_entity_by_name_with_transform(k_background_city_layer_name), 0.0f);
      rt.city_layers_hidden = true;
    }

    inline void init_city_scroll_if_needed(BossRuntime& rt)
    {
      if (!rt.city_scroll_enabled || rt.city_scroll_initialized) {
        return;
      }

      Math::Vec2D min_bounds, max_bounds;
      if (!get_tilemap_world_bounds(min_bounds, max_bounds)) {
        if (!rt.debug_logged_tilemap_bounds_missing) {
          Log::warning("CatMonkey: city scroll enabled but no tilemap bounds were found (no TileMap entities?).");
          rt.debug_logged_tilemap_bounds_missing = true;
        }
        return;
      }

      if (!rt.debug_logged_city_init) {
        Log::info("CatMonkey: initializing Summon5 city scroll. Tilemap bounds min=({},{}), max=({},{}).", min_bounds.x, min_bounds.y, max_bounds.x, max_bounds.y);
        rt.debug_logged_city_init = true;
      }

      auto init_layer = [&](const char* layer_name, float& out_start_x)
      {
        Engine::ECS::Entity e = find_entity_by_name_with_transform(layer_name);
        if (!e.is_valid()) {
          Log::warning("CatMonkey: city layer entity not found (missing Transform2D or name mismatch): '{}'", layer_name ? layer_name : "<null>");
          return;
        }

        // Detach so each prefab/layer can move independently.
        detach_from_parent_if_present(e);

        // Make visible.
        set_entity_alpha_if_present(e, 1.0f);

        auto* t = e.get_component<Components::Transform2D>();
        if (!t) {
          return;
        }

        const float half_w = std::max(0.0f, t->scale.x) * 0.5f;
        // Start just off the left edge so wide images don't instantly
        // satisfy the "hit right edge" reset condition.
        out_start_x = min_bounds.x - half_w;
        t->position.x = out_start_x;

        // Helpful one-time snapshot for debugging layer sizing/placement.
        Log::info("CatMonkey: city layer '{}' init pos.x={} scale=({},{}).", layer_name, t->position.x, t->scale.x, t->scale.y);
      };

      init_layer(k_cityscape_layer_name, rt.cityscape_start_x);
      init_layer(k_minicity_layer_name, rt.minicity_start_x);
      init_layer(k_foreground_city_layer_name, rt.foreground_city_start_x);
      init_layer(k_background_city_layer_name, rt.background_city_start_x);

      rt.city_scroll_initialized = true;
    }

    inline void tick_city_scroll_if_enabled(BossRuntime& rt)
    {
      if (!rt.city_scroll_enabled) {
        return;
      }

      init_city_scroll_if_needed(rt);
      if (!rt.city_scroll_initialized) {
        return;
      }

      Math::Vec2D min_bounds, max_bounds;
      if (!get_tilemap_world_bounds(min_bounds, max_bounds)) {
        return;
      }

      const float dt = Engine::Time::get_fixed_dt();
      if (dt <= 0.0f) {
        return;
      }

      auto tick_layer = [&](const char* layer_name, float speed, float start_x, bool reset_at_edge)
      {
        Engine::ECS::Entity e = find_entity_by_name_with_transform(layer_name);
        if (!e.is_valid()) {
          return;
        }

        auto* t = e.get_component<Components::Transform2D>();
        if (!t) {
          return;
        }

        t->position.x += speed * dt;

        const float half_w = std::max(0.0f, t->scale.x) * 0.5f;
        const float right_edge = t->position.x + half_w;
        const float left_edge = t->position.x - half_w;

        if (reset_at_edge) {
          // First two images: reset when they hit the edge of the tilemap.
          if (right_edge >= max_bounds.x) {
            t->position.x = start_x;
          }
        }
        else {
          // Last two images: reset only once fully out of the tilemap.
          if (left_edge >= max_bounds.x) {
            t->position.x = start_x;
          }
        }
      };

      tick_layer(k_cityscape_layer_name, k_cityscape_speed, rt.cityscape_start_x, true);
      tick_layer(k_minicity_layer_name, k_minicity_speed, rt.minicity_start_x, true);
      tick_layer(k_foreground_city_layer_name, k_foreground_city_speed, rt.foreground_city_start_x, false);
      tick_layer(k_background_city_layer_name, k_background_city_speed, rt.background_city_start_x, false);
    }

    inline void tick_summon5_cinematic_camera(BossRuntime& rt)
    {
      if (!rt.summon5_cinematic_active) {
        return;
      }

      const float now = Engine::Time::get_time();
      if (now >= rt.summon5_cinematic_end_time) {
        rt.summon5_cinematic_active = false;
        return;
      }

      Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
      if (!centered.is_valid()) {
        if (!rt.debug_logged_centered_camera_missing) {
          Log::warning("CatMonkey: Summon5 cinematic active but camera entity '{}' not found.", k_centered_camera_name);
          rt.debug_logged_centered_camera_missing = true;
        }
        return;
      }

      auto* t = centered.get_component<Components::Transform2D>();
      if (!t) {
        return;
      }

      const float dur = std::max(0.001f, rt.summon5_cinematic_end_time - rt.summon5_cinematic_start_time);
      float u = (now - rt.summon5_cinematic_start_time) / dur;
      u = std::clamp(u, 0.0f, 1.0f);
      // Smoothstep easing.
      u = u * u * (3.0f - 2.0f * u);

      const float target_zoom = std::clamp(k_summon5_target_zoom, 0.1f, 10.0f);
      const float z = rt.summon5_zoom_start + (target_zoom - rt.summon5_zoom_start) * u;

      t->position = rt.summon5_pos_start + (rt.summon5_pos_target - rt.summon5_pos_start) * u;
      t->rotation = rt.summon5_rot_start + (rt.summon5_rot_target - rt.summon5_rot_start) * u;
      t->scale.x = z;
      t->scale.y = z;
    }

    struct SpawnSetEntry
    {
      std::string prefab_path;
      Math::Vec2D world_pos;
    };

    inline std::unordered_map<Engine::ECS::EntityID, BossRuntime>& runtimes()
    {
      static std::unordered_map<Engine::ECS::EntityID, BossRuntime> map;
      return map;
    }

    inline BossRuntime& runtime_for(Engine::ECS::Entity boss)
    {
      return runtimes()[boss.get_id()];
    }

    inline void clear_runtime_for(Engine::ECS::Entity boss)
    {
      runtimes().erase(boss.get_id());
    }

    inline bool all_waves_cleared(const BossRuntime& rt)
    {
      return std::all_of(rt.wave_cleared.begin(), rt.wave_cleared.end(), [](bool v) { return v; });
    }

    // ------------------------------------------------------------------------
    // Config (kept in code for now)
    // ------------------------------------------------------------------------
    inline const Components::CatMonkeyBoss* get_cfg(Engine::ECS::Entity boss)
    {
      return boss.get_component<Components::CatMonkeyBoss>();
    }

    inline bool is_ai_disabled(Engine::ECS::Entity boss)
    {
      if (const Components::CatMonkeyBoss* cfg = get_cfg(boss)) {
        return cfg->ai_disabled;
      }
      return false;
    }

    inline Engine::ECS::Entity find_camera_entity_by_name(const char* name)
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

    inline void set_active_camera_entity(Engine::ECS::Entity camera_entity)
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

    inline void activate_player_camera_if_present()
    {
      Engine::ECS::Entity cam = find_camera_entity_by_name(k_player_camera_name);
      if (cam.is_valid()) {
        set_active_camera_entity(cam);
      }
    }

    inline void activate_cat_monkey_camera_if_present()
    {
      Engine::ECS::Entity cam = find_camera_entity_by_name(k_cat_monkey_camera_name);
      if (cam.is_valid()) {
        set_active_camera_entity(cam);
      }
    }

    inline void activate_centered_camera_if_present()
    {
      Engine::ECS::Entity cam = find_camera_entity_by_name(k_centered_camera_name);
      if (cam.is_valid()) {
        set_active_camera_entity(cam);
      }
    }

    inline int minions_per_summon_for(Engine::ECS::Entity boss, int summon_index)
    {
      const Components::CatMonkeyBoss* cfg = get_cfg(boss);
      if (!cfg) {
        return 0;
      }

      int per_wave = 0;
      if (summon_index == 0) per_wave = cfg->summon1_minions;
      else if (summon_index == 1) per_wave = cfg->summon2_minions;
      else if (summon_index == 2) per_wave = cfg->summon3_minions;
      else if (summon_index == 3) per_wave = cfg->summon4_minions;
      else if (summon_index == 4) per_wave = cfg->summon5_minions;

      if (per_wave > 0) {
        return per_wave;
      }

      return cfg->minions_per_summon;
    }

    inline float summon5_spawn_interval(Engine::ECS::Entity boss)
    {
      const Components::CatMonkeyBoss* cfg = get_cfg(boss);
      if (!cfg) {
        return 3.0f;
      }
      return std::max(0.1f, cfg->summon5_spawn_interval_sec);
    }

    inline const std::string& spawn_set_json_for(Engine::ECS::Entity boss, int summon_index)
    {
      static const std::string empty;
      static const std::string default_1 = "spawn_sets/summon1.json";
      static const std::string default_2 = "spawn_sets/summon2.json";
      static const std::string default_3 = "spawn_sets/summon3.json";
      static const std::string default_4 = "spawn_sets/summon4.json";
      static const std::string default_5 = "spawn_sets/summon5.json";

      const Components::CatMonkeyBoss* cfg = get_cfg(boss);
      if (cfg) {
        const std::string* configured = nullptr;
        if (summon_index == 0) configured = &cfg->summon1_spawn_set_json;
        else if (summon_index == 1) configured = &cfg->summon2_spawn_set_json;
        else if (summon_index == 2) configured = &cfg->summon3_spawn_set_json;
        else if (summon_index == 3) configured = &cfg->summon4_spawn_set_json;
        else if (summon_index == 4) configured = &cfg->summon5_spawn_set_json;

        if (configured && !configured->empty()) {
          return *configured;
        }
      }

      // Backwards-compat/default behavior: if the tuning fields aren't present in JSON
      // (e.g. prefab saved before the editor shows them), fall back to the standard
      // spawn_sets paths so Cat Monkey still uses the selection-export workflow.
      if (summon_index == 1) return default_2;
      if (summon_index == 2) return default_3;
      if (summon_index == 3) return default_4;
      if (summon_index == 4) return default_5;
      if (summon_index == 0) return default_1;
      return empty;
    }

    inline std::vector<SpawnSetEntry> load_spawn_set_entries(Engine::ECS::Entity boss, int summon_index)
    {
      std::vector<SpawnSetEntry> out;

      const std::string& rel_path = spawn_set_json_for(boss, summon_index);
      if (rel_path.empty()) {
        return out;
      }

      auto read_file_to_string = [](const std::string& path) -> std::optional<std::string>
      {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
          return std::nullopt;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
      };

      const std::string full_path = (std::filesystem::path(Engine::Resources::resource_path) / rel_path).string();
      if (!std::filesystem::exists(full_path)) {
        Log::warning("CatMonkey: spawn set json does not exist: {} (full: {})", rel_path, full_path);
        return out;
      }
      auto json_opt = read_file_to_string(full_path);
      if (!json_opt.has_value()) {
        Log::warning("CatMonkey: failed to read spawn set json file: {}", rel_path);
        return out;
      }

      auto gv_opt = ReflectionLibrary::generic_value_from_json(json_opt.value());
      if (!gv_opt.has_value() || !gv_opt->isObject()) {
        Log::warning("CatMonkey: invalid spawn set json (parse/object): {}", rel_path);
        return out;
      }

      const auto& root = gv_opt.value();
      if (!root.has("data") || !root["data"].isObject()) {
        Log::warning("CatMonkey: invalid spawn set json (missing data): {}", rel_path);
        return out;
      }

      const auto& data = root["data"];
      if (!data.has("entities") || !data["entities"].isArray()) {
        Log::warning("CatMonkey: invalid spawn set json (missing entities[]): {}", rel_path);
        return out;
      }

      const auto& entities = data["entities"].getArray();
      out.reserve(entities.size());
      for (const auto& e : entities)
      {
        if (!e.isObject()) {
          continue;
        }

        if (!e.has("prefab") || !e["prefab"].isString()) {
          continue;
        }
        const std::string prefab = e["prefab"].getString();
        if (prefab.empty()) {
          continue;
        }

        Math::Vec2D pos{ 0.0f, 0.0f };
        if (e.has("transform") && e["transform"].isObject())
        {
          const auto& tr = e["transform"];
          if (tr.has("position") && tr["position"].isObject())
          {
            const auto& p = tr["position"];
            if (p.has("x")) {
              if (p["x"].isDouble()) pos.x = static_cast<float>(p["x"].getDouble());
              else if (p["x"].isInt()) pos.x = static_cast<float>(p["x"].getInt());
            }
            if (p.has("y")) {
              if (p["y"].isDouble()) pos.y = static_cast<float>(p["y"].getDouble());
              else if (p["y"].isInt()) pos.y = static_cast<float>(p["y"].getInt());
            }
          }
        }

        out.push_back({ prefab, pos });
      }

      if (out.empty()) {
        Log::warning("CatMonkey: spawn set json parsed 0 entries: {}", rel_path);
      }
      else {
        Log::info("CatMonkey: spawn set json loaded {} entries: {}", out.size(), rel_path);
      }
      return out;
    }

    inline bool summon5_has_more_to_spawn(const BossRuntime& rt)
    {
      if (rt.summon5_timed_spawning) {
        return true;
      }
      return rt.summon5_next_index < rt.summon5_pending_prefabs.size();
    }

    inline Math::Vec2D summon5_spawn_pos_at(const BossRuntime& rt, size_t index)
    {
      if (rt.summon5_spawn_positions.empty()) {
        return { 0.0f, 0.0f };
      }
      if (rt.summon5_spawn_positions.size() == rt.summon5_pending_prefabs.size() && index < rt.summon5_spawn_positions.size()) {
        return rt.summon5_spawn_positions[index];
      }
      return rt.summon5_spawn_positions[index % rt.summon5_spawn_positions.size()];
    }

    struct Summon5SpawnItem
    {
      std::string prefab;
      Math::Vec2D pos{ 0.0f, 0.0f };
      bool has_pos = false;
    };

    inline uint32_t summon5_shuffle_seed(Engine::ECS::Entity boss)
    {
      // Seed per run, per boss instance; deterministic seeding isn't required for Summon5.
      const uint64_t id = static_cast<uint64_t>(boss.get_id());
      const uint64_t t = static_cast<uint64_t>(Engine::Time::get_time() * 1000.0f);
      return static_cast<uint32_t>((id ^ (t + 0x9E3779B97F4A7C15ull)) & 0xFFFFFFFFu);
    }

    inline std::vector<Summon5SpawnItem> interleave_items_by_prefab(std::vector<Summon5SpawnItem> items, std::mt19937& rng)
    {
      if (items.size() <= 2) {
        std::shuffle(items.begin(), items.end(), rng);
        return items;
      }

      std::unordered_map<std::string, std::vector<Summon5SpawnItem>> buckets;
      buckets.reserve(items.size());
      for (auto& it : items) {
        buckets[it.prefab].push_back(std::move(it));
      }

      for (auto& [_, bucket] : buckets) {
        std::shuffle(bucket.begin(), bucket.end(), rng);
      }

      struct Node
      {
        size_t remaining = 0;
        uint32_t tie = 0;
        std::string key;
      };

      auto cmp = [](const Node& a, const Node& b)
      {
        if (a.remaining != b.remaining) return a.remaining < b.remaining; // max-heap by remaining
        return a.tie < b.tie; // random-ish tiebreaker
      };

      std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
      pq = decltype(pq)(cmp);

      for (const auto& [key, bucket] : buckets) {
        pq.push(Node{ bucket.size(), rng(), key });
      }

      std::vector<Summon5SpawnItem> out;
      out.reserve(items.size());
      std::string last_key;

      while (!pq.empty())
      {
        Node first = pq.top();
        pq.pop();

        if (!last_key.empty() && first.key == last_key && !pq.empty())
        {
          Node second = pq.top();
          pq.pop();

          auto& b2 = buckets[second.key];
          out.push_back(std::move(b2.back()));
          b2.pop_back();
          last_key = second.key;

          if (!b2.empty()) {
            pq.push(Node{ b2.size(), rng(), second.key });
          }
          // Put the first choice back unchanged.
          pq.push(Node{ buckets[first.key].size(), rng(), first.key });
        }
        else
        {
          auto& b1 = buckets[first.key];
          out.push_back(std::move(b1.back()));
          b1.pop_back();
          last_key = first.key;

          if (!b1.empty()) {
            pq.push(Node{ b1.size(), rng(), first.key });
          }
        }
      }

      return out;
    }

    inline void randomize_summon5_spawn_order(
      Engine::ECS::Entity boss,
      std::vector<std::string>& prefabs,
      std::vector<Math::Vec2D>* positions)
    {
      if (prefabs.size() <= 1) {
        return;
      }

      const bool lockstep = (positions != nullptr) && (positions->size() == prefabs.size());

      std::mt19937 rng(summon5_shuffle_seed(boss));

      std::vector<Summon5SpawnItem> items;
      items.reserve(prefabs.size());

      if (lockstep)
      {
        for (size_t i = 0; i < prefabs.size(); ++i) {
          items.push_back(Summon5SpawnItem{ prefabs[i], (*positions)[i], true });
        }
      }
      else
      {
        for (const auto& p : prefabs) {
          items.push_back(Summon5SpawnItem{ p, { 0.0f, 0.0f }, false });
        }
      }

      // Interleave by prefab key to reduce long streaks (e.g. slingshot -> slingshot -> slingshot).
      items = interleave_items_by_prefab(std::move(items), rng);

      for (size_t i = 0; i < items.size(); ++i) {
        prefabs[i] = std::move(items[i].prefab);
        if (lockstep) {
          (*positions)[i] = items[i].pos;
        }
      }
    }

    inline int minions_per_summon(Engine::ECS::Entity boss)
    {
      if (auto* cfg = get_cfg(boss)) {
        return cfg->minions_per_summon;
      }
      return 6;
    }

    inline float summon_cooldown(Engine::ECS::Entity boss)
    {
      if (auto* cfg = get_cfg(boss)) {
        return cfg->summon_cooldown;
      }
      return 2.0f;
    }

    inline int damage_per_minion_death(Engine::ECS::Entity boss)
    {
      if (auto* cfg = get_cfg(boss)) {
        return cfg->damage_per_minion_death;
      }
      return 1;
    }

    inline const std::vector<std::string>& summon_prefabs_for(Engine::ECS::Entity boss, int summon_index)
    {
      const Components::CatMonkeyBoss* cfg = get_cfg(boss);
      if (cfg) {
        if (summon_index == 0 && !cfg->summon1_prefabs.empty()) return cfg->summon1_prefabs;
        if (summon_index == 1 && !cfg->summon2_prefabs.empty()) return cfg->summon2_prefabs;
        if (summon_index == 2 && !cfg->summon3_prefabs.empty()) return cfg->summon3_prefabs;
        if (summon_index == 3 && !cfg->summon4_prefabs.empty()) return cfg->summon4_prefabs;
        if (summon_index == 4 && !cfg->summon5_prefabs.empty()) return cfg->summon5_prefabs;
      }

      static const std::vector<std::string> empty;
      return empty;
    }

    inline const std::string& spawn_marker_prefix_for(Engine::ECS::Entity boss, int summon_index)
    {
      static const std::string default_1 = "cat_monkey_spawn1";
      static const std::string default_2 = "cat_monkey_spawn2";
      static const std::string default_3 = "cat_monkey_spawn3";
      static const std::string default_4 = "cat_monkey_spawn4";
      static const std::string default_5 = "cat_monkey_spawn5";

      const Components::CatMonkeyBoss* cfg = get_cfg(boss);
      if (cfg) {
        if (summon_index == 0 && !cfg->summon1_spawn_marker_prefix.empty()) return cfg->summon1_spawn_marker_prefix;
        if (summon_index == 1 && !cfg->summon2_spawn_marker_prefix.empty()) return cfg->summon2_spawn_marker_prefix;
        if (summon_index == 2 && !cfg->summon3_spawn_marker_prefix.empty()) return cfg->summon3_spawn_marker_prefix;
        if (summon_index == 3 && !cfg->summon4_spawn_marker_prefix.empty()) return cfg->summon4_spawn_marker_prefix;
        if (summon_index == 4 && !cfg->summon5_spawn_marker_prefix.empty()) return cfg->summon5_spawn_marker_prefix;
      }

      if (summon_index == 1) return default_2;
      if (summon_index == 2) return default_3;
      if (summon_index == 3) return default_4;
      if (summon_index == 4) return default_5;
      return default_1;
    }

    inline const std::vector<Math::Vec2D>& spawn_offsets_for(Engine::ECS::Entity boss, int summon_index)
    {
      static const std::vector<Math::Vec2D> empty;

      const Components::CatMonkeyBoss* cfg = get_cfg(boss);
      if (!cfg) {
        return empty;
      }

      if (summon_index == 0) return cfg->summon1_spawn_offsets;
      if (summon_index == 1) return cfg->summon2_spawn_offsets;
      if (summon_index == 2) return cfg->summon3_spawn_offsets;
      if (summon_index == 3) return cfg->summon4_spawn_offsets;
      if (summon_index == 4) return cfg->summon5_spawn_offsets;
      return empty;
    }

    inline bool starts_with(const std::string& str, const std::string& prefix)
    {
      if (prefix.empty()) {
        return false;
      }
      if (str.size() < prefix.size()) {
        return false;
      }
      return std::equal(prefix.begin(), prefix.end(), str.begin());
    }

    inline std::vector<Math::Vec2D> gather_spawn_positions_from_markers(const std::string& marker_prefix)
    {
      std::vector<std::pair<std::string, Math::Vec2D>> named;
      if (marker_prefix.empty()) {
        return {};
      }

      Engine::ECS::for_each_entity_with<Components::Transform2D>(
        [&](Engine::ECS::Entity e)
        {
          const std::string& name = e.get_name();
          if (!starts_with(name, marker_prefix)) {
            return;
          }

          auto* t = e.get_component<Components::Transform2D>();
          if (!t) {
            return;
          }

          named.emplace_back(name, t->position);
        });

      std::sort(named.begin(), named.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

      std::vector<Math::Vec2D> positions;
      positions.reserve(named.size());
      for (auto& it : named) {
        positions.push_back(it.second);
      }
      return positions;
    }

    inline void apply_boss_damage_from_minion_death(Engine::ECS::Entity boss_entity, int damage)
    {
      // IMPORTANT:
      // Cat Monkey's 5 bottom healthbars represent summon-wave progress.
      // Minion deaths should NOT reduce the boss' own EnemyHealth, otherwise
      // the boss can enter Death as soon as the first wave clears.
      (void)boss_entity;
      (void)damage;
    }

    inline int process_tracked_minion_deaths(Engine::ECS::Entity boss_entity, std::vector<Engine::ECS::EntityID>& tracked_summons)
    {
      if (tracked_summons.empty()) {
        return 0;
      }

      std::vector<Engine::ECS::EntityID> still_alive;
      still_alive.reserve(tracked_summons.size());

      int death_count = 0;

      for (Engine::ECS::EntityID id : tracked_summons)
      {
        Engine::ECS::Entity summoned(id);

        // If deactivated (e.g. via level preloading/scene lifecycle), don't count as dead.
        if (!Engine::ECS::is_entity_active(summoned)) {
          if (Engine::ECS::is_entity_deactivated(id)) {
            still_alive.push_back(id);
            continue;
          }

          apply_boss_damage_from_minion_death(boss_entity, damage_per_minion_death(boss_entity));
          death_count++;
          continue;
        }

        // If the enemy is still active but already at 0 HP, count it now.
        if (auto* hp = summoned.get_component<Components::EnemyHealth>()) {
          if (hp->current_hp <= 0) {
            apply_boss_damage_from_minion_death(boss_entity, damage_per_minion_death(boss_entity));
            death_count++;
            continue;
          }
        }

        still_alive.push_back(id);
      }

      tracked_summons = std::move(still_alive);
      return death_count;
    }

    inline int compute_summon_pool_current_hp(const std::vector<Engine::ECS::EntityID>& tracked_summons)
    {
      if (tracked_summons.empty()) {
        return 0;
      }

      int total = 0;
      for (Engine::ECS::EntityID id : tracked_summons) {
        Engine::ECS::Entity summoned(id);

        if (!Engine::ECS::is_entity_active(summoned)) {
          continue;
        }

        if (auto* hp = summoned.get_component<Components::EnemyHealth>()) {
          total += std::max(0, hp->current_hp);
        }
      }
      return total;
    }

    inline void ensure_wave_bars_component(Engine::ECS::Entity boss_entity)
    {
      if (boss_entity.get_component<Components::CatMonkeySummonWaveHealthBars>()) {
        return;
      }
      boss_entity.add_component<Components::CatMonkeySummonWaveHealthBars>();
    }

    inline void mark_wave_cleared(Engine::ECS::Entity boss_entity, int wave_index)
    {
      if (wave_index < 0 || wave_index >= k_summon_state_count) {
        return;
      }

      BossRuntime& rt = runtime_for(boss_entity);
      if (!rt.wave_spawned[static_cast<size_t>(wave_index)] || rt.wave_cleared[static_cast<size_t>(wave_index)]) {
        return;
      }

      rt.wave_cleared[static_cast<size_t>(wave_index)] = true;
      rt.wave_summons[static_cast<size_t>(wave_index)].clear();

      ensure_wave_bars_component(boss_entity);
      if (auto* bars = boss_entity.get_component<Components::CatMonkeySummonWaveHealthBars>()) {
        const float now = Engine::Time::get_time();
        if (wave_index == 0) { bars->wave1_cleared = true; bars->wave1_cleared_time = now; }
        if (wave_index == 1) { bars->wave2_cleared = true; bars->wave2_cleared_time = now; }
        if (wave_index == 2) { bars->wave3_cleared = true; bars->wave3_cleared_time = now; }
        if (wave_index == 3) { bars->wave4_cleared = true; bars->wave4_cleared_time = now; }
        if (wave_index == 4) { bars->wave5_cleared = true; bars->wave5_cleared_time = now; }
      }
    }

    inline void update_wave_healthbars_component(Engine::ECS::Entity boss_entity)
    {
      ensure_wave_bars_component(boss_entity);
      auto* bars = boss_entity.get_component<Components::CatMonkeySummonWaveHealthBars>();
      if (!bars) {
        return;
      }

      // Hide if boss dead.
      if (auto* boss_hp = boss_entity.get_component<Components::EnemyHealth>()) {
        if (boss_hp->current_hp <= 0) {
          bars->visible = false;
          return;
        }
      }

      bars->visible = true;

      BossRuntime& rt = runtime_for(boss_entity);

      auto set_wave = [&](int idx,
        int& cur, int& mx,
        bool& spawned, bool& cleared)
      {
        spawned = rt.wave_spawned[static_cast<size_t>(idx)];
        cleared = rt.wave_cleared[static_cast<size_t>(idx)];
        mx = std::max(0, rt.wave_pool_max_hp[static_cast<size_t>(idx)]);
        if (!spawned) {
          cur = 0;
          mx = 0;
          return;
        }
        if (cleared) {
          cur = 0;
          return;
        }

        auto& list = rt.wave_summons[static_cast<size_t>(idx)];
        cur = compute_summon_pool_current_hp(list);

        if (list.empty()) {
          if (idx == 4) {
            // Summon5 can have timed spawns; don't clear until all planned spawns are done.
            if (!summon5_has_more_to_spawn(rt)) {
              mark_wave_cleared(boss_entity, idx);
            }
          }
          else {
            mark_wave_cleared(boss_entity, idx);
          }
        }
      };

      set_wave(0, bars->wave1_current_hp, bars->wave1_max_hp, bars->wave1_spawned, bars->wave1_cleared);
      set_wave(1, bars->wave2_current_hp, bars->wave2_max_hp, bars->wave2_spawned, bars->wave2_cleared);
      set_wave(2, bars->wave3_current_hp, bars->wave3_max_hp, bars->wave3_spawned, bars->wave3_cleared);
      set_wave(3, bars->wave4_current_hp, bars->wave4_max_hp, bars->wave4_spawned, bars->wave4_cleared);
      set_wave(4, bars->wave5_current_hp, bars->wave5_max_hp, bars->wave5_spawned, bars->wave5_cleared);
    }

    inline void update_summon_healthbar_component(Engine::ECS::Entity boss_entity)
    {
      auto* bar = boss_entity.get_component<Components::CatMonkeySummonHealthBar>();
      if (!bar) {
        boss_entity.add_component<Components::CatMonkeySummonHealthBar>();
        bar = boss_entity.get_component<Components::CatMonkeySummonHealthBar>();
      }
      if (!bar) {
        return;
      }

      // Show the bar whenever the boss is alive (even if empty).
      if (auto* boss_hp = boss_entity.get_component<Components::EnemyHealth>()) {
        if (boss_hp->current_hp <= 0) {
          bar->visible = false;
          bar->total_current_hp = 0;
          bar->total_max_hp = 0;
          return;
        }
      }

      bar->visible = true;

      // Legacy rectangle bar is no longer used for waves; keep it empty.

      // Otherwise, keep it visible but empty.
      bar->total_current_hp = 0;
      bar->total_max_hp = 0;
    }

    inline void restart_entity_animation(Engine::ECS::Entity entity)
    {
      if (auto* anim = entity.get_component<Components::Animation>()) {
        anim->startTime = Engine::Time::get_time();
      }
      if (auto* atlas = entity.get_component<Components::TextureAtlas>()) {
        atlas->drawIndex = 0;
      }
    }

    inline bool has_animation_finished(Engine::ECS::Entity entity)
    {
      auto* anim = entity.get_component<Components::Animation>();
      if (!anim) {
        return true;
      }

      // Normal non-looping path.
      if (!anim->is_looping) {
        return anim->has_ended();
      }

      // Safety: if looping, treat "one full cycle" as finished.
      const float dur = anim->totalDuration;
      if (dur <= 0.0f) {
        return false;
      }

      const float elapsed = Engine::Time::get_time() - anim->startTime;
      return elapsed >= dur;
    }

    inline void spawn_wave_for_summon_index(Engine::ECS::Entity entity, int summon_index)
    {
      auto* sm = entity.get_component<Components::StateMachine>();
      auto* hp = entity.get_component<Components::EnemyHealth>();
      if (!sm || !hp) {
        return;
      }

      if (hp->current_hp <= 0) {
        return;
      }

      const std::string& spawn_set_path = spawn_set_json_for(entity, summon_index);
      if (spawn_set_path.empty()) {
        Log::warning("CatMonkey: Summon{} spawn_set_json is empty; using legacy spawn markers/offsets.", summon_index + 1);
      }
      else {
        Log::info("CatMonkey: Summon{} spawn_set_json='{}'", summon_index + 1, spawn_set_path);
      }

      // If a spawn-set JSON file is configured, it overrides the legacy spawn logic.
      const std::vector<SpawnSetEntry> spawn_set = load_spawn_set_entries(entity, summon_index);
      if (!spawn_set_path.empty() && spawn_set.empty()) {
        // Force visibility of misconfiguration: don't silently fall back.
        Log::warning("CatMonkey: Summon{} spawn-set configured but produced 0 spawns; NOT falling back to legacy.", summon_index + 1);
        return;
      }

      int minion_count = 0;
      const std::vector<std::string>* prefabs_ptr = nullptr;
      std::vector<Math::Vec2D> spawn_positions;

      if (!spawn_set.empty()) {
        minion_count = static_cast<int>(spawn_set.size());
      }
      else {
        minion_count = minions_per_summon_for(entity, summon_index);
        if (minion_count <= 0) {
          return;
        }

        const auto& prefabs = summon_prefabs_for(entity, summon_index);
        if (prefabs.empty()) {
          Log::warning("CatMonkey: summon prefabs list is empty (configure Components::CatMonkeyBoss in JSON).");
          return;
        }
        prefabs_ptr = &prefabs;

        spawn_positions = gather_spawn_positions_from_markers(spawn_marker_prefix_for(entity, summon_index));
        if (spawn_positions.empty()) {
          const std::vector<Math::Vec2D>& offsets = spawn_offsets_for(entity, summon_index);
          if (auto* t = entity.get_component<Components::Transform2D>()) {
            if (!offsets.empty()) {
              spawn_positions.reserve(offsets.size());
              for (const Math::Vec2D& off : offsets) {
                spawn_positions.push_back(t->position + off);
              }
            }
          }
        }

        if (spawn_positions.empty()) {
          Log::warning("CatMonkey: no spawn positions available (no markers found and no spawn offsets configured). (You can also configure summonX_spawn_set_json to use editor-exported spawn sets.)");
          return;
        }
      }

      BossRuntime& rt = runtime_for(entity);
      if (summon_index < 0 || summon_index >= k_summon_state_count) {
        return;
      }

      auto& summons = rt.wave_summons[static_cast<size_t>(summon_index)];
      summons.clear();
      summons.reserve(static_cast<size_t>(minion_count));
      rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = 0;
      rt.wave_spawned[static_cast<size_t>(summon_index)] = true;
      rt.wave_cleared[static_cast<size_t>(summon_index)] = false;
      rt.active_wave_index = summon_index;

      // Reset Summon5 timed spawn bookkeeping each time we spawn a wave.
      rt.summon5_timed_spawning = false;
      rt.summon5_pending_prefabs.clear();
      rt.summon5_spawn_positions.clear();
      rt.summon5_next_index = 0;
      rt.summon5_next_spawn_time = 0.0f;
      rt.summon5_current_spawn_interval_sec = 5.0f;

      // Spawn-set JSON path: spawn exactly what was exported.
      if (!spawn_set.empty())
      {
        if (summon_index == 4)
        {
          rt.summon5_pending_prefabs.reserve(spawn_set.size());
          rt.summon5_spawn_positions.reserve(spawn_set.size());
          for (const auto& it : spawn_set) {
            rt.summon5_pending_prefabs.push_back(it.prefab_path);
            rt.summon5_spawn_positions.push_back(it.world_pos);
          }

          // Summon5: randomize spawn ORDER so enemy types don't clump.
          randomize_summon5_spawn_order(entity, rt.summon5_pending_prefabs, &rt.summon5_spawn_positions);

          if (!rt.summon5_pending_prefabs.empty()) {
            // Spawn first immediately, then tick the rest.
            const std::string& prefab_path = rt.summon5_pending_prefabs[0];
            const Math::Vec2D spawn_pos = summon5_spawn_pos_at(rt, 0);
            Engine::ECS::Entity summoned = spawn_enemy_from_prefab(prefab_path, spawn_pos);
            if (summoned.is_valid()) {
              summons.push_back(summoned.get_id());
              if (auto* mhp = summoned.get_component<Components::EnemyHealth>()) {
                rt.wave_pool_max_hp[4] += std::max(0, mhp->max_hp);
              }
            }

            rt.summon5_next_index = 1;
            rt.summon5_next_spawn_time = Engine::Time::get_time() + rt.summon5_current_spawn_interval_sec;
            rt.summon5_timed_spawning = (rt.summon5_next_index < rt.summon5_pending_prefabs.size());
          }
        }
        else
        {
          for (size_t i = 0; i < spawn_set.size(); ++i)
          {
            const auto& it = spawn_set[i];
            Engine::ECS::Entity summoned = spawn_enemy_from_prefab(it.prefab_path, it.world_pos);
            if (summoned.is_valid()) {
              summons.push_back(summoned.get_id());
              if (auto* mhp = summoned.get_component<Components::EnemyHealth>()) {
                rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] += std::max(0, mhp->max_hp);
              }
            }
          }
        }

        update_wave_healthbars_component(entity);
        return;
      }

      auto spawn_one_from_list = [&](const std::string& prefab_path, int i, const std::vector<Math::Vec2D>& positions)
      {
        const Math::Vec2D& spawn_pos = positions[static_cast<size_t>(i) % positions.size()];
        Engine::ECS::Entity summoned = spawn_enemy_from_prefab(prefab_path, spawn_pos);
        if (summoned.is_valid()) {
          summons.push_back(summoned.get_id());
          if (auto* mhp = summoned.get_component<Components::EnemyHealth>()) {
            rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] += std::max(0, mhp->max_hp);
          }
        }
      };

      if (summon_index == 4) {
        // Summon5 legacy path: cycle prefab list and marker/offset positions.
        rt.summon5_timed_spawning = true;
        rt.summon5_pending_prefabs.reserve(static_cast<size_t>(minion_count));
        rt.summon5_spawn_positions = spawn_positions;
        rt.summon5_next_index = 0;
        rt.summon5_next_spawn_time = Engine::Time::get_time();

        const auto& prefabs = *prefabs_ptr;
        for (int i = 0; i < minion_count; ++i) {
          rt.summon5_pending_prefabs.push_back(prefabs[static_cast<size_t>(i) % prefabs.size()]);
        }

        // Summon5: randomize spawn ORDER so enemy types don't clump.
        randomize_summon5_spawn_order(entity, rt.summon5_pending_prefabs, nullptr);

        if (!rt.summon5_pending_prefabs.empty() && !rt.summon5_spawn_positions.empty()) {
          spawn_one_from_list(rt.summon5_pending_prefabs[0], 0, rt.summon5_spawn_positions);
          rt.summon5_next_index = 1;
          rt.summon5_next_spawn_time = Engine::Time::get_time() + rt.summon5_current_spawn_interval_sec;
        }
      }
      else {
        for (int i = 0; i < minion_count; ++i)
        {
          const auto& prefabs = *prefabs_ptr;
          const std::string& prefab_path = prefabs[static_cast<size_t>(i) % prefabs.size()];
          spawn_one_from_list(prefab_path, i, spawn_positions);
        }
      }

      update_wave_healthbars_component(entity);
    }

    inline Engine::ECS::Entity spawn_enemy_from_prefab(const std::string& prefab_path, const Math::Vec2D& pos)
    {
      auto prefab = Engine::Resources::load_prefab(prefab_path);
      Engine::ECS::Entity enemy = Engine::ECS::instantiate_prefab(prefab);

      if (auto* t = enemy.get_component<Components::Transform2D>()) {
        t->position = pos;
      }

      return enemy;
    }
  }

  // ========================================================================
  // Null State Callbacks (do nothing)
  // ========================================================================

  inline void null_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // Dev-safe: if we were in the middle of a summon camera override,
    // restore the normal player camera.
    if (entity.is_valid()) {
      BossRuntime& rt = runtime_for(entity);
      rt.camera_override_active = false;
      rt.camera_override_until = 0.0f;
    }
    activate_player_camera_if_present();
  }

  inline void null_on_update([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // Intentionally empty.
  }

  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // If the dev toggle is enabled, keep the boss inert.
    if (is_ai_disabled(entity)) {
      if (auto* sm = entity.get_component<Components::StateMachine>()) {
        sm->change_state("Null");
      }
    }

    // Ensure we don't instantly start summoning on level start.
    BossRuntime& rt = runtime_for(entity);

    // Prep city layers (hide until Summon5 is reached).
    ensure_city_layers_hidden(rt);

    if (!rt.summon_pacing_initialized) {
      rt.summon_pacing_initialized = true;
      rt.next_summon_allowed_time = Engine::Time::get_time() + summon_cooldown(entity);
    }
  }

  inline void idle_on_update(Engine::ECS::Entity entity)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();

    if (!sm || !hp) {
      return;
    }

    if (is_ai_disabled(entity)) {
      sm->change_state("Null");
      return;
    }

    // Boss already dead.
    if (hp->current_hp <= 0) {
      return;
    }

    BossRuntime& rt = runtime_for(entity);

    // If Summon5 has been reached, keep scrolling city layers even while idling.
    tick_city_scroll_if_enabled(rt);

    // If we are in a post-summon camera override window, keep the Cat_Monkey camera active.
    // Once the timer elapses, restore the player camera.
    const float now = Engine::Time::get_time();

    // Summon pacing init (covers cases where runtime is created from idle_on_update first).
    if (!rt.summon_pacing_initialized) {
      rt.summon_pacing_initialized = true;
      rt.next_summon_allowed_time = now + summon_cooldown(entity);
    }

    if (rt.camera_override_active) {
      if (now < rt.camera_override_until) {
        if (rt.camera_override_mode == BossRuntime::CameraOverrideMode::Centered) {
          activate_centered_camera_if_present();
          tick_summon5_cinematic_camera(rt);
        }
        else {
          activate_cat_monkey_camera_if_present();
        }
      }
      else {
        // End of override window: restore the normal player camera.
        rt.camera_override_active = false;
        rt.camera_override_until = 0.0f;
        rt.summon5_cinematic_active = false;
        activate_player_camera_if_present();
      }
    }

    // Summon5 staggered spawns continue while the wave is active.
    if (rt.active_wave_index == 4 && rt.summon5_timed_spawning) {
      if (!rt.summon5_pending_prefabs.empty() && !rt.summon5_spawn_positions.empty()) {
        if (now >= rt.summon5_next_spawn_time && rt.summon5_next_index < rt.summon5_pending_prefabs.size()) {
          auto& summons = rt.wave_summons[4];
          const size_t i = rt.summon5_next_index;
          const std::string& prefab_path = rt.summon5_pending_prefabs[i];
          const Math::Vec2D spawn_pos = summon5_spawn_pos_at(rt, i);
          Engine::ECS::Entity summoned = spawn_enemy_from_prefab(prefab_path, spawn_pos);
          if (summoned.is_valid()) {
            summons.push_back(summoned.get_id());
            if (auto* mhp = summoned.get_component<Components::EnemyHealth>()) {
              rt.wave_pool_max_hp[4] += std::max(0, mhp->max_hp);
            }
          }

          rt.summon5_next_index++;
          rt.summon5_next_spawn_time = now + rt.summon5_current_spawn_interval_sec;
          if (rt.summon5_next_index >= rt.summon5_pending_prefabs.size()) {
            rt.summon5_timed_spawning = false;
          }
        }
      }
      else {
        rt.summon5_timed_spawning = false;
      }
    }

    if (rt.active_wave_index >= 0 && rt.active_wave_index < k_summon_state_count) {
      auto& list = rt.wave_summons[static_cast<size_t>(rt.active_wave_index)];
      const int deaths = process_tracked_minion_deaths(entity, list);

      // Summon5: ramp spawn speed up after each kill.
      if (rt.active_wave_index == 4 && deaths > 0 && summon5_has_more_to_spawn(rt)) {
        // Start at 5s, reduce 10% per kill until reaching 1s.
        static constexpr float k_min_interval = 1.0f;
        static constexpr float k_per_kill_multiplier = 0.9f;
        for (int i = 0; i < deaths; ++i) {
          rt.summon5_current_spawn_interval_sec = std::max(k_min_interval, rt.summon5_current_spawn_interval_sec * k_per_kill_multiplier);
        }

        // Pull the next spawn earlier (never later) so the speed-up feels immediate.
        const float earliest_next = now + rt.summon5_current_spawn_interval_sec;
        if (rt.summon5_next_spawn_time > earliest_next) {
          rt.summon5_next_spawn_time = earliest_next;
        }
      }

      if (list.empty()) {
        if (rt.active_wave_index == 4) {
          // Don't clear Summon5 until all planned spawns have been spawned.
          if (!summon5_has_more_to_spawn(rt)) {
            mark_wave_cleared(entity, rt.active_wave_index);
            rt.active_wave_index = -1;

            // Add a chill window after the set is fully cleared.
            rt.next_summon_allowed_time = std::max(rt.next_summon_allowed_time, now + summon_cooldown(entity));
          }
        }
        else {
          mark_wave_cleared(entity, rt.active_wave_index);
          rt.active_wave_index = -1;

          // Add a chill window after the set is cleared.
          rt.next_summon_allowed_time = std::max(rt.next_summon_allowed_time, now + summon_cooldown(entity));
        }
      }
    }
    update_wave_healthbars_component(entity);

    // If no active summons remain, and cooldown elapsed, summon again.
    // Placeholder end-state behavior: once all 5 summon waves have been cleared,
    // the boss should stop cycling Summon1..Summon5 and simply idle.
    if (all_waves_cleared(rt)) {
      return;
    }

    bool cooldown_ready = now >= rt.next_summon_allowed_time;
    if (rt.active_wave_index == -1 && cooldown_ready) {
      const int idx = (rt.next_summon_index % k_summon_state_count + k_summon_state_count) % k_summon_state_count;
      if (idx == 1) sm->change_state("Summon2");
      else if (idx == 2) sm->change_state("Summon3");
      else if (idx == 3) sm->change_state("Summon4");
      else if (idx == 4) sm->change_state("Summon5");
      else sm->change_state("Summon1");
    }
  }

  // ========================================================================
  // Summon State Callbacks (Summon1 / Summon2 / Summon3)
  // ========================================================================

  inline void summon_impl_on_enter(Engine::ECS::Entity entity, int summon_index)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();

    if (!sm || !hp) {
      return;
    }

    if (is_ai_disabled(entity)) {
      sm->change_state("Null");
      return;
    }

    if (hp->current_hp <= 0) {
      return;
    }

    BossRuntime& rt = runtime_for(entity);
    rt.last_summon_time = Engine::Time::get_time();
    rt.next_summon_index = (rt.next_summon_index + 1) % k_summon_state_count;

    // Mark wave spawn as pending; actual spawn happens after the animation finishes.
    rt.summon_pending = true;
    rt.pending_summon_index = summon_index;
    if (summon_index >= 0 && summon_index < k_summon_state_count) {
      rt.wave_summons[static_cast<size_t>(summon_index)].clear();
      rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = 0;
    }

    restart_entity_animation(entity);
    update_wave_healthbars_component(entity);

    // Camera: swap immediately to the Cat_Monkey camera when summoning starts.
    // Keep it active at least through the summon animation.
    rt.camera_override_active = true;
    rt.camera_override_until = std::numeric_limits<float>::infinity();
    rt.camera_override_mode = BossRuntime::CameraOverrideMode::CatMonkey;
    rt.summon5_cinematic_active = false;
    activate_cat_monkey_camera_if_present();
  }

  inline void summon1_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 0); }
  inline void summon2_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 1); }
  inline void summon3_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 2); }
  inline void summon4_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 3); }
  inline void summon5_on_enter(Engine::ECS::Entity entity)
  {
    summon_impl_on_enter(entity, 4);
    BossRuntime& rt = runtime_for(entity);
    rt.city_scroll_enabled = true;
    // Note: initialization + visibility happens lazily on the next tick.

    if (!rt.debug_logged_summon5_enter) {
      Log::info("CatMonkey: entered Summon5 state. Enabling city scroll + Centered camera cinematic.");
      rt.debug_logged_summon5_enter = true;
    }
  }

  inline void summon_impl_on_update(Engine::ECS::Entity entity)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();
    if (!sm || !hp) {
      return;
    }

    if (is_ai_disabled(entity)) {
      sm->change_state("Null");
      return;
    }

    if (hp->current_hp <= 0) {
      return;
    }

    BossRuntime& rt = runtime_for(entity);

    // If Summon5 has been reached, scroll the city layers during summon animations too.
    tick_city_scroll_if_enabled(rt);
    update_wave_healthbars_component(entity);

    if (!rt.summon_pending) {
      // If something cleared the flag, just go back to Idle.
      sm->change_state("Idle");
      return;
    }

    if (!has_animation_finished(entity)) {
      return;
    }

    spawn_wave_for_summon_index(entity, rt.pending_summon_index);
    rt.summon_pending = false;

    // Enforce chill time before the next summon can begin.
    // (Cooldown starts after the summon animation finishes.)
    const float now = Engine::Time::get_time();
    rt.next_summon_allowed_time = now + summon_cooldown(entity);

    if (rt.pending_summon_index == 4)
    {
      // Summon5: switch to the Centered camera and slowly zoom out so the
      // city layers are visible and feel like a train background passing by.
      Engine::ECS::Entity centered = find_camera_entity_by_name(k_centered_camera_name);
      Math::Vec2D target_pos{};
      float target_rot = 0.0f;
      if (centered.is_valid()) {
        if (auto* t = centered.get_component<Components::Transform2D>()) {
          target_pos = t->position;
          target_rot = t->rotation;
        }
      }

      const Engine::Graphics::Camera current = Engine::Graphics::get_camera();
      if (centered.is_valid()) {
        if (auto* t = centered.get_component<Components::Transform2D>()) {
          // Start from the current render camera state to avoid a snap, then
          // ease toward the authored Centered camera transform while zooming out.
          t->position = current.position;
          t->rotation = current.rotation;
          t->scale.x = current.zoom;
          t->scale.y = current.zoom;
        }
      }

      rt.camera_override_active = true;
      rt.camera_override_mode = BossRuntime::CameraOverrideMode::Centered;
      // Keep Centered camera strictly active after Summon5 (no snap back to player).
      rt.camera_override_until = std::numeric_limits<float>::infinity();

      rt.summon5_cinematic_active = true;
      rt.summon5_cinematic_start_time = now;
      rt.summon5_cinematic_end_time = now + k_summon5_cinematic_duration_sec;
      rt.summon5_zoom_start = current.zoom;
      rt.summon5_pos_start = current.position;
      rt.summon5_rot_start = current.rotation;
      rt.summon5_pos_target = target_pos;
      rt.summon5_rot_target = target_rot;

      activate_centered_camera_if_present();
      tick_summon5_cinematic_camera(rt);
    }
    else
    {
      // Other summons: linger briefly on the Cat_Monkey camera so the player
      // can see the wave appear.
      rt.camera_override_active = true;
      rt.camera_override_mode = BossRuntime::CameraOverrideMode::CatMonkey;
      rt.camera_override_until = now + k_post_summon_camera_linger_sec;
      activate_cat_monkey_camera_if_present();
    }

    // Go back to idle to wait for the wave to be cleared.
    sm->change_state("Idle");
  }

  inline void summon1_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  inline void summon2_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  inline void summon3_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  inline void summon4_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }
  inline void summon5_on_update(Engine::ECS::Entity entity) { summon_impl_on_update(entity); }

  // Backwards-compat: legacy "Summon" state redirects to Summon1.
  inline void summon_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 0); }
  inline void summon_on_update([[maybe_unused]] Engine::ECS::Entity entity) {}

  // ========================================================================
  // Hit / Death (minimal, for engine damage callback compatibility)
  // ========================================================================

  inline void hit_on_enter(Engine::ECS::Entity entity)
  {
    if (auto* sm = entity.get_component<Components::StateMachine>()) {
      sm->change_state("Idle");
    }
  }

  inline void death_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // If you later add a death animation, keep this state as the sink.
    clear_runtime_for(entity);

    if (auto* bar = entity.get_component<Components::CatMonkeySummonHealthBar>()) {
      bar->visible = false;
      bar->total_current_hp = 0;
      bar->total_max_hp = 0;
    }

    if (auto* bars = entity.get_component<Components::CatMonkeySummonWaveHealthBars>()) {
      bars->visible = false;
    }

    // Restore player camera if this boss was holding an override.
    activate_player_camera_if_present();
  }

  inline void death_on_update(Engine::ECS::Entity entity)
  {
    // Destroy after death animation ends if present, otherwise destroy immediately.
    if (auto* anim = entity.get_component<Components::Animation>()) {
      if (!anim->has_ended()) {
        return;
      }
    }

    Engine::ECS::destroy_entity(entity);
  }

} // namespace CatMonkeyStateCallbacks

// ============================================================================
// Backwards-compat callback aliases
// ============================================================================
namespace {
  void Cat_monkey_idle_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::idle_on_enter(entity); }
  void Cat_monkey_idle_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::idle_on_update(entity); }
  void Cat_monkey_null_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::null_on_enter(entity); }
  void Cat_monkey_null_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::null_on_update(entity); }
  void Cat_monkey_summon_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon_on_enter(entity); }
  void Cat_monkey_summon_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon_on_update(entity); }
  void Cat_monkey_summon1_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon1_on_enter(entity); }
  void Cat_monkey_summon1_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon1_on_update(entity); }
  void Cat_monkey_summon2_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon2_on_enter(entity); }
  void Cat_monkey_summon2_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon2_on_update(entity); }
  void Cat_monkey_summon3_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon3_on_enter(entity); }
  void Cat_monkey_summon3_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon3_on_update(entity); }
  void Cat_monkey_summon4_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon4_on_enter(entity); }
  void Cat_monkey_summon4_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon4_on_update(entity); }
  void Cat_monkey_summon5_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon5_on_enter(entity); }
  void Cat_monkey_summon5_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::summon5_on_update(entity); }
  void Cat_monkey_hit_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::hit_on_enter(entity); }
  void Cat_monkey_death_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::death_on_enter(entity); }
  void Cat_monkey_death_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::death_on_update(entity); }
}

REGISTER_FUNCTION(Cat_monkey_idle_enter)
REGISTER_FUNCTION(Cat_monkey_idle_update)
REGISTER_FUNCTION(Cat_monkey_null_enter)
REGISTER_FUNCTION(Cat_monkey_null_update)
REGISTER_FUNCTION(Cat_monkey_summon_enter)
REGISTER_FUNCTION(Cat_monkey_summon_update)
REGISTER_FUNCTION(Cat_monkey_summon1_enter)
REGISTER_FUNCTION(Cat_monkey_summon1_update)
REGISTER_FUNCTION(Cat_monkey_summon2_enter)
REGISTER_FUNCTION(Cat_monkey_summon2_update)
REGISTER_FUNCTION(Cat_monkey_summon3_enter)
REGISTER_FUNCTION(Cat_monkey_summon3_update)
REGISTER_FUNCTION(Cat_monkey_summon4_enter)
REGISTER_FUNCTION(Cat_monkey_summon4_update)
REGISTER_FUNCTION(Cat_monkey_summon5_enter)
REGISTER_FUNCTION(Cat_monkey_summon5_update)
REGISTER_FUNCTION(Cat_monkey_hit_enter)
REGISTER_FUNCTION(Cat_monkey_death_enter)
REGISTER_FUNCTION(Cat_monkey_death_update)

REGISTER_STATE_MACHINE_BLUEPRINT(CatMonkey, "Idle",
  STATE("Null", CatMonkeyStateCallbacks::null_on_enter, CatMonkeyStateCallbacks::null_on_update, NO_CALLBACK)
  STATE("Idle", CatMonkeyStateCallbacks::idle_on_enter, CatMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Summon1", CatMonkeyStateCallbacks::summon1_on_enter, CatMonkeyStateCallbacks::summon1_on_update, NO_CALLBACK)
  STATE("Summon2", CatMonkeyStateCallbacks::summon2_on_enter, CatMonkeyStateCallbacks::summon2_on_update, NO_CALLBACK)
  STATE("Summon3", CatMonkeyStateCallbacks::summon3_on_enter, CatMonkeyStateCallbacks::summon3_on_update, NO_CALLBACK)
  STATE("Summon4", CatMonkeyStateCallbacks::summon4_on_enter, CatMonkeyStateCallbacks::summon4_on_update, NO_CALLBACK)
  STATE("Summon5", CatMonkeyStateCallbacks::summon5_on_enter, CatMonkeyStateCallbacks::summon5_on_update, NO_CALLBACK)
  STATE("Hit", CatMonkeyStateCallbacks::hit_on_enter, NO_CALLBACK, NO_CALLBACK)
  STATE("Death", CatMonkeyStateCallbacks::death_on_enter, CatMonkeyStateCallbacks::death_on_update, NO_CALLBACK)
)

// Optional: explicit registration entrypoint (called from RegisterGameLogic).
// This guarantees the blueprint exists even if static auto-registration is skipped.
void register_catmonkey_blueprint_runtime()
{
  auto& registry = ::Engine::StateMachineBlueprintRegistry::instance();
  ::Engine::StateMachineBlueprint bp;
  bp.initial_state = "Idle";
  STATE("Null", CatMonkeyStateCallbacks::null_on_enter, CatMonkeyStateCallbacks::null_on_update, NO_CALLBACK)
  STATE("Idle", CatMonkeyStateCallbacks::idle_on_enter, CatMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Summon1", CatMonkeyStateCallbacks::summon1_on_enter, CatMonkeyStateCallbacks::summon1_on_update, NO_CALLBACK)
  STATE("Summon2", CatMonkeyStateCallbacks::summon2_on_enter, CatMonkeyStateCallbacks::summon2_on_update, NO_CALLBACK)
  STATE("Summon3", CatMonkeyStateCallbacks::summon3_on_enter, CatMonkeyStateCallbacks::summon3_on_update, NO_CALLBACK)
  STATE("Summon4", CatMonkeyStateCallbacks::summon4_on_enter, CatMonkeyStateCallbacks::summon4_on_update, NO_CALLBACK)
  STATE("Summon5", CatMonkeyStateCallbacks::summon5_on_enter, CatMonkeyStateCallbacks::summon5_on_update, NO_CALLBACK)
  STATE("Hit", CatMonkeyStateCallbacks::hit_on_enter, NO_CALLBACK, NO_CALLBACK)
  STATE("Death", CatMonkeyStateCallbacks::death_on_enter, CatMonkeyStateCallbacks::death_on_update, NO_CALLBACK)
  registry.register_blueprint("CatMonkey", bp);
}

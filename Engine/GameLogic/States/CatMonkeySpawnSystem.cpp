//-----------------------------------------------------------------------------------
// file:    CatMonkeySpawnSystem.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   System and helpers for spawning Cat Monkey boss fight summons. Loads spawn sets
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "CatMonkeySpawnSystem.h"
#include "CatMonkeyBossRuntime.h"
#include "../Components/CatMonkeyBossComponent.h"
#include "../Components/EnemyHealthComponent.h"
#include "../../Engine/Components/UtilityComponents.h"
#include "../../Engine/Reflection/ReflectionLibrary.h"

namespace CatMonkeyStateCallbacks
{
  namespace {
    std::string to_lower_ascii(std::string s)
    {
      for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
      }
      return s;
    }

    std::optional<std::string> spawn_cloud_prefab_for_enemy_prefab(const std::string& enemy_prefab_path)
    {
      if (enemy_prefab_path.empty()) return std::nullopt;

      std::string filename = std::filesystem::path(enemy_prefab_path).filename().string();
      filename = to_lower_ascii(std::move(filename));

      if (filename == "turret_monkey.json") return std::string{"prefabs/spawn_cloud_purple.json"};
      if (filename == "spray can monkey.json") return std::string{"prefabs/spawn_cloud_pink.json"};
      if (filename == "slingshot_monkey.json") return std::string{"prefabs/spawn_cloud_yellow.json"};
      if (filename == "lazy_monkey.json") return std::string{"prefabs/spawn_cloud_blue.json"};

      return std::nullopt;
    }

    void spawn_spawn_cloud_if_configured(const std::string& enemy_prefab_path, const Math::Vec2D& pos)
    {
      auto cloud_path = spawn_cloud_prefab_for_enemy_prefab(enemy_prefab_path);
      if (!cloud_path) return;

      Engine::ECS::Prefab cloud_prefab = Engine::Resources::load_prefab(*cloud_path);
      if (!cloud_prefab.is_valid()) {
        Log::warning("CatMonkey: failed to load spawn cloud prefab {}", *cloud_path);
        return;
      }

      Engine::ECS::Entity cloud = Engine::ECS::instantiate_prefab(cloud_prefab);
      if (!cloud.is_valid()) return;

      if (auto* t = cloud.get_component<Components::Transform2D>()) {
        t->position = pos;
      }

      // Ensure the animation starts at spawn time (prefabs often store editor timestamps).
      float lifetime_sec = 1.0f;
      if (auto* anim = cloud.get_component<Components::Animation>()) {
        anim->startTime = Engine::Time::get_time();
        if (anim->totalDuration > 0.0f) {
          lifetime_sec = std::max(0.05f, anim->totalDuration + 0.05f);
        }
      }

      if (!cloud.has_component<Components::Lifetime>()) {
        cloud.add_component<Components::Lifetime>(lifetime_sec);
      }
    }

    int max_hp_from_prefab(const std::string& prefab_path)
    {
      if (prefab_path.empty()) return 0;
      Engine::ECS::Prefab prefab = Engine::Resources::load_prefab(prefab_path);
      if (!prefab.is_valid()) return 0;
      if (const auto* hp = prefab.get_component<Components::EnemyHealth>()) {
        return std::max(0, hp->max_hp);
      }
      return 0;
    }

    const Components::CatMonkeyBoss* get_cfg(Engine::ECS::Entity boss)
    {
      return boss.get_component<Components::CatMonkeyBoss>();
    }

    int minions_per_summon_for(Engine::ECS::Entity boss, int summon_index)
    {
      const auto* cfg = get_cfg(boss);
      if (!cfg) return 0;

      int per_wave = 0;
      switch (summon_index) {
        case 0: per_wave = cfg->summon1_minions; break;
        case 1: per_wave = cfg->summon2_minions; break;
        case 2: per_wave = cfg->summon3_minions; break;
        case 3: per_wave = cfg->summon4_minions; break;
        case 4: per_wave = cfg->summon5_minions; break;
        default: break;
      }
      return per_wave > 0 ? per_wave : cfg->minions_per_summon;
    }

    float summon5_spawn_interval_for(Engine::ECS::Entity boss)
    {
      const auto* cfg = get_cfg(boss);
      return (cfg && cfg->summon5_spawn_interval_sec > 0.0f) ? cfg->summon5_spawn_interval_sec : 3.0f;
    }

    const std::vector<std::string>* summon_prefabs_for(const Components::CatMonkeyBoss& cfg, int summon_index)
    {
      switch (summon_index) {
        case 0: return &cfg.summon1_prefabs;
        case 1: return &cfg.summon2_prefabs;
        case 2: return &cfg.summon3_prefabs;
        case 3: return &cfg.summon4_prefabs;
        case 4: return &cfg.summon5_prefabs;
        default: return nullptr;
      }
    }

    const std::string* spawn_marker_prefix_for(const Components::CatMonkeyBoss& cfg, int summon_index)
    {
      switch (summon_index) {
        case 0: return &cfg.summon1_spawn_marker_prefix;
        case 1: return &cfg.summon2_spawn_marker_prefix;
        case 2: return &cfg.summon3_spawn_marker_prefix;
        case 3: return &cfg.summon4_spawn_marker_prefix;
        case 4: return &cfg.summon5_spawn_marker_prefix;
        default: return nullptr;
      }
    }

    const std::vector<Math::Vec2D>* spawn_offsets_for(const Components::CatMonkeyBoss& cfg, int summon_index)
    {
      switch (summon_index) {
        case 0: return &cfg.summon1_spawn_offsets;
        case 1: return &cfg.summon2_spawn_offsets;
        case 2: return &cfg.summon3_spawn_offsets;
        case 3: return &cfg.summon4_spawn_offsets;
        case 4: return &cfg.summon5_spawn_offsets;
        default: return nullptr;
      }
    }

    std::vector<Math::Vec2D> gather_spawn_positions_from_markers(const std::string& prefix)
    {
      std::vector<std::pair<std::string, Math::Vec2D>> found;

      if (prefix.empty()) {
        return {};
      }

      Engine::ECS::for_each_entity_with<Components::Transform2D>(
        [&](Engine::ECS::Entity e)
        {
          const std::string name = e.get_name();
          if (name.rfind(prefix, 0) != 0) {
            return;
          }

          if (auto* t = e.get_component<Components::Transform2D>()) {
            found.push_back({name, t->position});
          }
        });

      std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
      });

      std::vector<Math::Vec2D> out;
      out.reserve(found.size());
      for (const auto& it : found) {
        out.push_back(it.second);
      }
      return out;
    }

    std::vector<Math::Vec2D> build_spawn_positions_from_offsets(const Math::Vec2D& origin, const std::vector<Math::Vec2D>& offsets)
    {
      std::vector<Math::Vec2D> out;
      out.reserve(offsets.size());
      for (const auto& off : offsets) {
        out.push_back(origin + off);
      }
      return out;
    }

    std::vector<Math::Vec2D> build_simple_fallback_positions(const Math::Vec2D& origin, int count)
    {
      std::vector<Math::Vec2D> out;
      if (count <= 0) return out;

      out.reserve(static_cast<size_t>(count));
      const float spacing = 80.0f;
      const float half = 0.5f * static_cast<float>(count - 1);
      for (int i = 0; i < count; ++i) {
        const float x = (static_cast<float>(i) - half) * spacing;
        out.push_back({origin.x + x, origin.y});
      }
      return out;
    }

    Math::Vec2D boss_world_pos(Engine::ECS::Entity boss)
    {
      if (auto* t = boss.get_component<Components::Transform2D>()) {
        return t->position;
      }
      return {};
    }
  }

  std::vector<SpawnSetEntry> load_spawn_set_entries(Engine::ECS::Entity boss, int summon_index)
  {
    std::vector<SpawnSetEntry> out;

    const auto* cfg = boss.get_component<Components::CatMonkeyBoss>();
    if (!cfg) return out;

    std::string rel_path;
    if (summon_index == 0) rel_path = cfg->summon1_spawn_set_json;
    else if (summon_index == 1) rel_path = cfg->summon2_spawn_set_json;
    else if (summon_index == 2) rel_path = cfg->summon3_spawn_set_json;
    else if (summon_index == 3) rel_path = cfg->summon4_spawn_set_json;
    else if (summon_index == 4) rel_path = cfg->summon5_spawn_set_json;

    if (rel_path.empty()) return out;

    auto read_file = [](const std::string& path) -> std::optional<std::string> {
      std::filesystem::path rel = std::filesystem::path(path);
      std::filesystem::path base = std::filesystem::path(Engine::Resources::resource_path);

      std::vector<std::filesystem::path> candidates;
      candidates.reserve(4);

      if (rel.is_absolute()) {
        candidates.push_back(rel);
      }

      candidates.push_back(base / rel);

      const bool base_is_assets = (!base.empty() && base.filename() == "Assets");
      const bool rel_starts_with_assets = (!rel.empty() && rel.begin() != rel.end() && rel.begin()->string() == "Assets");
      if (!base_is_assets && !rel_starts_with_assets) {
        candidates.push_back(base / "Assets" / rel);
      }

      for (const auto& full : candidates) {
        std::ifstream file(full.string(), std::ios::binary);
        if (!file) continue;
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
      }

      std::ostringstream tried;
      for (size_t i = 0; i < candidates.size(); ++i) {
        tried << candidates[i].string();
        if (i + 1 < candidates.size()) tried << " | ";
      }
      Log::warning("CatMonkey: failed to read file '{}' (resource_path='{}'). Tried: {}", path, Engine::Resources::resource_path, tried.str());
      return std::nullopt;
    };

    auto json_opt = read_file(rel_path);
    if (!json_opt) {
      return out;
    }

    auto gv_opt = ReflectionLibrary::generic_value_from_json(*json_opt);
    if (!gv_opt || !gv_opt->isObject() || !(*gv_opt)["data"].isObject() || !(*gv_opt)["data"]["entities"].isArray()) {
      Log::warning("CatMonkey: invalid spawn set JSON: {}", rel_path);
      return out;
    }

    const auto& entities = (*gv_opt)["data"]["entities"].getArray();
    out.reserve(entities.size());

    for (const auto& e : entities) {
      if (!e.isObject() || !e["prefab"].isString()) continue;
      const std::string prefab = e["prefab"].getString();
      if (prefab.empty()) continue;

      Math::Vec2D pos{0};
      if (e["transform"]["position"].isObject()) {
        const auto& p = e["transform"]["position"];
        if (p["x"].isDouble()) pos.x = static_cast<float>(p["x"].getDouble());
        else if (p["x"].isInt()) pos.x = static_cast<float>(p["x"].getInt());

        if (p["y"].isDouble()) pos.y = static_cast<float>(p["y"].getDouble());
        else if (p["y"].isInt()) pos.y = static_cast<float>(p["y"].getInt());
      }

      out.push_back({prefab, pos});
    }

    Log::info("CatMonkey: loaded {} spawn set entries from {}", out.size(), rel_path);
    return out;
  }

  Engine::ECS::Entity spawn_enemy_from_prefab(const std::string& prefab_path, const Math::Vec2D& pos)
  {
    auto prefab = Engine::Resources::load_prefab(prefab_path);
    if (!prefab.is_valid()) {
      Log::warning("CatMonkey: failed to load prefab {}", prefab_path);
      return {};
    }

    Engine::ECS::Entity enemy = Engine::ECS::instantiate_prefab(prefab);
    if (auto* t = enemy.get_component<Components::Transform2D>()) {
      t->position = pos;
    }

    if (enemy.is_valid()) {
      spawn_spawn_cloud_if_configured(prefab_path, pos);
    }

    return enemy;
  }

  bool summon5_has_more_to_spawn(const BossRuntime& rt)
  {
    return rt.summon5_timed_spawning || rt.summon5_next_index < rt.summon5_pending_prefabs.size();
  }

  Math::Vec2D summon5_spawn_pos_at(const BossRuntime& rt, size_t index)
  {
    if (rt.summon5_spawn_positions.empty()) return {};
    if (index < rt.summon5_spawn_positions.size()) return rt.summon5_spawn_positions[index];
    return rt.summon5_spawn_positions[index % rt.summon5_spawn_positions.size()];
  }

  uint32_t summon5_shuffle_seed(Engine::ECS::Entity boss)
  {
    const uint64_t id = static_cast<uint64_t>(boss.get_id());
    const uint64_t t = static_cast<uint64_t>(Engine::Time::get_time() * 1000.0f);
    return static_cast<uint32_t>((id ^ (t + 0x9E3779B97F4A7C15ull)) & 0xFFFFFFFFu);
  }

  std::vector<Summon5SpawnItem> interleave_items_by_prefab(std::vector<Summon5SpawnItem> items, std::mt19937& rng)
  {
    if (items.size() <= 2) {
      std::shuffle(items.begin(), items.end(), rng);
      return items;
    }

    std::unordered_map<std::string, std::vector<Summon5SpawnItem>> buckets;
    for (auto& it : items) {
      buckets[it.prefab].push_back(std::move(it));
    }

    for (auto& [_, bucket] : buckets) {
      std::shuffle(bucket.begin(), bucket.end(), rng);
    }

    struct Node {
      size_t remaining = 0;
      uint32_t tie = 0;
      std::string key;
    };

    auto cmp = [](const Node& a, const Node& b) {
      if (a.remaining != b.remaining) return a.remaining < b.remaining;
      return a.tie < b.tie;
    };

    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);

    for (const auto& [key, bucket] : buckets) {
      pq.push({bucket.size(), rng(), key});
    }

    std::vector<Summon5SpawnItem> out;
    out.reserve(items.size());
    std::string last_key;

    while (!pq.empty()) {
      Node first = pq.top();
      pq.pop();

      if (!last_key.empty() && first.key == last_key && !pq.empty()) {
        Node second = pq.top();
        pq.pop();

        auto& b2 = buckets[second.key];
        out.push_back(std::move(b2.back()));
        b2.pop_back();
        last_key = second.key;

        if (!b2.empty()) pq.push({b2.size(), rng(), second.key});
        pq.push({buckets[first.key].size(), rng(), first.key});
      } else {
        auto& b1 = buckets[first.key];
        out.push_back(std::move(b1.back()));
        b1.pop_back();
        last_key = first.key;

        if (!b1.empty()) pq.push({b1.size(), rng(), first.key});
      }
    }

    return out;
  }

  void randomize_summon5_spawn_order(Engine::ECS::Entity boss, std::vector<std::string>& prefabs, std::vector<Math::Vec2D>* positions, std::vector<int>* max_hps)
  {
    if (prefabs.size() <= 1) return;

    const bool lockstep = positions && positions->size() == prefabs.size();
    const bool lockstep_hp = max_hps && max_hps->size() == prefabs.size();
    std::mt19937 rng(summon5_shuffle_seed(boss));

    std::vector<Summon5SpawnItem> items;
    items.reserve(prefabs.size());

    if (lockstep || lockstep_hp) {
      for (size_t i = 0; i < prefabs.size(); ++i) {
        Summon5SpawnItem it;
        it.prefab = prefabs[i];
        it.has_pos = lockstep;
        if (lockstep) it.pos = (*positions)[i];
        it.max_hp = lockstep_hp ? (*max_hps)[i] : 0;
        items.push_back(std::move(it));
      }
    } else {
      for (const auto& p : prefabs) {
        items.push_back({p, {}, false});
      }
    }

    items = interleave_items_by_prefab(std::move(items), rng);

    for (size_t i = 0; i < items.size(); ++i) {
      prefabs[i] = std::move(items[i].prefab);
      if (lockstep) (*positions)[i] = items[i].pos;
      if (lockstep_hp) (*max_hps)[i] = items[i].max_hp;
    }
  }

  void spawn_wave_for_summon_index(Engine::ECS::Entity entity, int summon_index)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();
    if (!sm || !hp || hp->current_hp <= 0) return;

    BossRuntime& rt = runtime_for(entity);
    if (summon_index < 0 || summon_index >= k_summon_state_count) return;

    auto& summons = rt.wave_summons[static_cast<size_t>(summon_index)];
    summons.clear();
    rt.wave_spawned[static_cast<size_t>(summon_index)] = true;
    rt.wave_cleared[static_cast<size_t>(summon_index)] = false;
    rt.active_wave_index = summon_index;
    rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = 0;

    // Reset Summon5 state
    rt.summon5_timed_spawning = false;
    rt.summon5_pending_prefabs.clear();
    rt.summon5_spawn_positions.clear();
    rt.summon5_pending_max_hp.clear();
    rt.summon5_pending_hp_remaining = 0;
    rt.summon5_next_index = 0;
    rt.summon5_next_spawn_time = 0.0f;
    rt.summon5_current_spawn_interval_sec = summon5_spawn_interval_for(entity);

    // Load spawn set
    const std::vector<SpawnSetEntry> spawn_set = load_spawn_set_entries(entity, summon_index);
    if (!spawn_set.empty()) {
      Log::info("CatMonkey: spawning summon{} from spawn_set_json ({} entries)", summon_index + 1, spawn_set.size());

      int planned_total_hp = 0;
      std::vector<int> planned_max_hp;
      planned_max_hp.reserve(spawn_set.size());
      for (const auto& it : spawn_set) {
        const int mhp = max_hp_from_prefab(it.prefab_path);
        planned_total_hp += mhp;
        planned_max_hp.push_back(mhp);
      }
      rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = std::max(0, planned_total_hp);

      if (summon_index == 4) {
        // Summon5 timed spawn from spawn_set
        rt.summon5_pending_prefabs.reserve(spawn_set.size());
        rt.summon5_spawn_positions.reserve(spawn_set.size());
        rt.summon5_pending_max_hp.reserve(spawn_set.size());
        for (const auto& it : spawn_set) {
          rt.summon5_pending_prefabs.push_back(it.prefab_path);
          rt.summon5_spawn_positions.push_back(it.world_pos);
          rt.summon5_pending_max_hp.push_back(planned_max_hp[rt.summon5_pending_max_hp.size()]);
        }
        randomize_summon5_spawn_order(entity, rt.summon5_pending_prefabs, &rt.summon5_spawn_positions, &rt.summon5_pending_max_hp);

        rt.summon5_pending_hp_remaining = rt.wave_pool_max_hp[4];
        if (!rt.summon5_pending_prefabs.empty()) {
          // Spawn first immediately
          const int first_mhp = (rt.summon5_pending_max_hp.size() > 0) ? rt.summon5_pending_max_hp[0] : max_hp_from_prefab(rt.summon5_pending_prefabs[0]);
          Engine::ECS::Entity first = spawn_enemy_from_prefab(rt.summon5_pending_prefabs[0], summon5_spawn_pos_at(rt, 0));
          rt.summon5_pending_hp_remaining = std::max(0, rt.summon5_pending_hp_remaining - std::max(0, first_mhp));
          if (first.is_valid()) {
            summons.push_back(first.get_id());
          }
          rt.summon5_next_index = 1;
          rt.summon5_next_spawn_time = Engine::Time::get_time() + rt.summon5_current_spawn_interval_sec;
          rt.summon5_timed_spawning = true;
        }
      } else {
        // Instant spawn all
        for (size_t i = 0; i < spawn_set.size(); ++i) {
          const auto& it = spawn_set[i];
          Engine::ECS::Entity summoned = spawn_enemy_from_prefab(it.prefab_path, it.world_pos);
          if (summoned.is_valid()) {
            summons.push_back(summoned.get_id());
          } else {
            // If a spawn fails, remove its contribution so the bar still matches reality.
            rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = std::max(0, rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] - planned_max_hp[i]);
          }
        }
      }
    } else {
      // Legacy spawn logic (prefab lists + markers/offsets)
      const auto* cfg = get_cfg(entity);
      if (!cfg) {
        Log::warning("CatMonkey: missing CatMonkeyBoss component; cannot spawn wave {}", summon_index + 1);
        return;
      }

      const auto* prefabs = summon_prefabs_for(*cfg, summon_index);
      if (!prefabs || prefabs->empty()) {
        Log::warning("CatMonkey: summon{} has no prefabs configured (and no spawn_set_json); nothing to spawn", summon_index + 1);
        return;
      }

      const int minion_count = std::max(0, minions_per_summon_for(entity, summon_index));
      if (minion_count <= 0) {
        Log::warning("CatMonkey: summon{} minion count is 0; nothing to spawn", summon_index + 1);
        return;
      }

      const Math::Vec2D origin = boss_world_pos(entity);
      std::vector<Math::Vec2D> positions;

      if (const auto* prefix = spawn_marker_prefix_for(*cfg, summon_index)) {
        positions = gather_spawn_positions_from_markers(*prefix);
      }

      if (positions.empty()) {
        if (const auto* offsets = spawn_offsets_for(*cfg, summon_index); offsets && !offsets->empty()) {
          positions = build_spawn_positions_from_offsets(origin, *offsets);
        }
      }

      if (positions.empty()) {
        positions = build_simple_fallback_positions(origin, minion_count);
      }

      auto prefab_at = [&](int i) -> const std::string& {
        return (*prefabs)[static_cast<size_t>(i) % prefabs->size()];
      };
      auto pos_at = [&](int i) -> Math::Vec2D {
        return positions[static_cast<size_t>(i) % positions.size()];
      };

      // Precompute max HP for this wave from the planned spawns.
      int planned_total_hp = 0;
      for (int i = 0; i < minion_count; ++i) {
        planned_total_hp += max_hp_from_prefab(prefab_at(i));
      }
      rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = std::max(0, planned_total_hp);

      if (summon_index == 4) {
        // Summon5 timed spawning: enqueue and spawn the first immediately.
        rt.summon5_pending_prefabs.reserve(static_cast<size_t>(minion_count));
        rt.summon5_spawn_positions.reserve(static_cast<size_t>(minion_count));
        rt.summon5_pending_max_hp.reserve(static_cast<size_t>(minion_count));
        for (int i = 0; i < minion_count; ++i) {
          rt.summon5_pending_prefabs.push_back(prefab_at(i));
          rt.summon5_spawn_positions.push_back(pos_at(i));
          rt.summon5_pending_max_hp.push_back(max_hp_from_prefab(prefab_at(i)));
        }

        randomize_summon5_spawn_order(entity, rt.summon5_pending_prefabs, &rt.summon5_spawn_positions, &rt.summon5_pending_max_hp);

        rt.summon5_pending_hp_remaining = rt.wave_pool_max_hp[4];

        if (!rt.summon5_pending_prefabs.empty()) {
          const int first_mhp = (rt.summon5_pending_max_hp.size() > 0) ? rt.summon5_pending_max_hp[0] : max_hp_from_prefab(rt.summon5_pending_prefabs[0]);
          Engine::ECS::Entity first = spawn_enemy_from_prefab(rt.summon5_pending_prefabs[0], summon5_spawn_pos_at(rt, 0));
          rt.summon5_pending_hp_remaining = std::max(0, rt.summon5_pending_hp_remaining - std::max(0, first_mhp));
          if (first.is_valid()) {
            summons.push_back(first.get_id());
          }

          rt.summon5_next_index = 1;
          rt.summon5_next_spawn_time = Engine::Time::get_time() + rt.summon5_current_spawn_interval_sec;
          rt.summon5_timed_spawning = true;
        }
      } else {
        // Summon 1-4: instant spawn all minions.
        for (int i = 0; i < minion_count; ++i) {
          Engine::ECS::Entity summoned = spawn_enemy_from_prefab(prefab_at(i), pos_at(i));
          if (summoned.is_valid()) {
            summons.push_back(summoned.get_id());
          } else {
            // If a spawn fails, remove its contribution so the bar still matches reality.
            rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] = std::max(0, rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] - max_hp_from_prefab(prefab_at(i)));
          }
        }
      }
    }
  }
}


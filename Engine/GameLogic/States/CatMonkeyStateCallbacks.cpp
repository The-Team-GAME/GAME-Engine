//-----------------------------------------------------------------------------------
// file:    CatMonkeyStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// brief:   Cat Monkey boss state machine callback implementations (serializable)
//-----------------------------------------------------------------------------------

#include "pch.h"
#include "../Components/EnemyHealthComponent.h"
#include "../Components/CatMonkeyBossComponent.h"
#include "../Components/CatMonkeySummonHealthBarComponent.h"
#include "../Components/CatMonkeySummonWaveHealthBarsComponent.h"

namespace CatMonkeyStateCallbacks
{
  namespace
  {
    static constexpr int k_summon_state_count = 5;

    inline Engine::ECS::Entity spawn_enemy_from_prefab(const std::string& prefab_path, const Math::Vec2D& pos);

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

    // ------------------------------------------------------------------------
    // Config (kept in code for now)
    // ------------------------------------------------------------------------
    inline const Components::CatMonkeyBoss* get_cfg(Engine::ECS::Entity boss)
    {
      return boss.get_component<Components::CatMonkeyBoss>();
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
      return 6.0f;
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
      std::vector<Math::Vec2D> positions;
      if (marker_prefix.empty()) {
        return positions;
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

          positions.push_back(t->position);
        });

      return positions;
    }

    inline void apply_boss_damage_from_minion_death(Engine::ECS::Entity boss_entity, int damage)
    {
      if (damage <= 0) {
        return;
      }

      auto* boss_hp = boss_entity.get_component<Components::EnemyHealth>();
      auto* boss_sm = boss_entity.get_component<Components::StateMachine>();

      if (!boss_hp) {
        return;
      }

      boss_hp->take_damage(damage);

      if (boss_hp->current_hp <= 0 && boss_sm) {
        if (boss_sm->get_current_state() != "Death") {
          boss_sm->change_state("Death");
        }
      }
    }

    inline void process_tracked_minion_deaths(Engine::ECS::Entity boss_entity, std::vector<Engine::ECS::EntityID>& tracked_summons)
    {
      if (tracked_summons.empty()) {
        return;
      }

      std::vector<Engine::ECS::EntityID> still_alive;
      still_alive.reserve(tracked_summons.size());

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
          continue;
        }

        // If the enemy is still active but already at 0 HP, count it now.
        if (auto* hp = summoned.get_component<Components::EnemyHealth>()) {
          if (hp->current_hp <= 0) {
            apply_boss_damage_from_minion_death(boss_entity, damage_per_minion_death(boss_entity));
            continue;
          }
        }

        still_alive.push_back(id);
      }

      tracked_summons = std::move(still_alive);
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
          mark_wave_cleared(boss_entity, idx);
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

      const int minion_count = minions_per_summon(entity);
      if (minion_count <= 0) {
        return;
      }

      const auto& prefabs = summon_prefabs_for(entity, summon_index);
      if (prefabs.empty()) {
        Log::warning("CatMonkey: summon prefabs list is empty (configure Components::CatMonkeyBoss in JSON).");
        return;
      }

      std::vector<Math::Vec2D> spawn_positions = gather_spawn_positions_from_markers(spawn_marker_prefix_for(entity, summon_index));
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
        Log::warning("CatMonkey: no spawn positions available (no markers found and no spawn offsets configured).");
        return;
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

      for (int i = 0; i < minion_count; ++i)
      {
        const std::string& prefab_path = prefabs[static_cast<size_t>(i) % prefabs.size()];
        const Math::Vec2D& spawn_pos = spawn_positions[static_cast<size_t>(i) % spawn_positions.size()];

        Engine::ECS::Entity summoned = spawn_enemy_from_prefab(prefab_path, spawn_pos);
        if (summoned.is_valid()) {
          summons.push_back(summoned.get_id());

          if (auto* mhp = summoned.get_component<Components::EnemyHealth>()) {
            rt.wave_pool_max_hp[static_cast<size_t>(summon_index)] += std::max(0, mhp->max_hp);
          }
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
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // Intentionally empty.
  }

  inline void idle_on_update(Engine::ECS::Entity entity)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();

    if (!sm || !hp) {
      return;
    }

    // Boss already dead.
    if (hp->current_hp <= 0) {
      return;
    }

    // Tick minion linkage for the currently-active wave.
    BossRuntime& rt = runtime_for(entity);
    if (rt.active_wave_index >= 0 && rt.active_wave_index < k_summon_state_count) {
      auto& list = rt.wave_summons[static_cast<size_t>(rt.active_wave_index)];
      process_tracked_minion_deaths(entity, list);
      if (list.empty()) {
        mark_wave_cleared(entity, rt.active_wave_index);
        rt.active_wave_index = -1;
      }
    }
    update_wave_healthbars_component(entity);

    // If no active summons remain, and cooldown elapsed, summon again.
    float now = Engine::Time::get_time();
    bool cooldown_ready = (now - rt.last_summon_time) >= summon_cooldown(entity);
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
  }

  inline void summon1_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 0); }
  inline void summon2_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 1); }
  inline void summon3_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 2); }
  inline void summon4_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 3); }
  inline void summon5_on_enter(Engine::ECS::Entity entity) { summon_impl_on_enter(entity, 4); }

  inline void summon_impl_on_update(Engine::ECS::Entity entity)
  {
    auto* sm = entity.get_component<Components::StateMachine>();
    auto* hp = entity.get_component<Components::EnemyHealth>();
    if (!sm || !hp) {
      return;
    }

    if (hp->current_hp <= 0) {
      return;
    }

    BossRuntime& rt = runtime_for(entity);
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

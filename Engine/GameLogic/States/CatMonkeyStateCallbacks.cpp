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

namespace CatMonkeyStateCallbacks
{
  namespace
  {
    struct BossRuntime
    {
      float last_summon_time = -9999.0f;
      std::vector<Engine::ECS::EntityID> active_summons{};
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
    inline int minions_per_summon()
    {
      return 6;
    }

    inline float summon_cooldown()
    {
      return 6.0f;
    }

    inline int damage_per_minion_death()
    {
      return 1;
    }

    inline const std::vector<std::string>& summon_prefabs()
    {
      static const std::vector<std::string> prefabs = {
        "prefabs/lazy_monkey.json",
        "prefabs/spray can monkey.json",
        "prefabs/slingshot_monkey.json",
        "prefabs/turret_monkey.json"
      };
      return prefabs;
    }

    inline const std::string& spawn_marker_prefix()
    {
      static const std::string prefix = "cat_monkey_spawn";
      return prefix;
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

    inline void process_tracked_minion_deaths(Engine::ECS::Entity boss_entity)
    {
      BossRuntime& rt = runtime_for(boss_entity);
      if (rt.active_summons.empty()) {
        return;
      }

      std::vector<Engine::ECS::EntityID> still_alive;
      still_alive.reserve(rt.active_summons.size());

      for (Engine::ECS::EntityID id : rt.active_summons)
      {
        Engine::ECS::Entity summoned(id);

        // If deactivated (e.g. via level preloading/scene lifecycle), don't count as dead.
        if (!Engine::ECS::is_entity_active(summoned)) {
          if (Engine::ECS::is_entity_deactivated(id)) {
            still_alive.push_back(id);
            continue;
          }

          apply_boss_damage_from_minion_death(boss_entity, damage_per_minion_death());
          continue;
        }

        // If the enemy is still active but already at 0 HP, count it now.
        if (auto* hp = summoned.get_component<Components::EnemyHealth>()) {
          if (hp->current_hp <= 0) {
            apply_boss_damage_from_minion_death(boss_entity, damage_per_minion_death());
            continue;
          }
        }

        still_alive.push_back(id);
      }

      rt.active_summons = std::move(still_alive);
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

    // Tick minion linkage.
    process_tracked_minion_deaths(entity);

    BossRuntime& rt = runtime_for(entity);

    // If no active summons remain, and cooldown elapsed, summon again.
    float now = Engine::Time::get_time();
    bool cooldown_ready = (now - rt.last_summon_time) >= summon_cooldown();
    if (rt.active_summons.empty() && cooldown_ready) {
      sm->change_state("Summon");
    }
  }

  // ========================================================================
  // Summon State Callbacks
  // ========================================================================

  inline void summon_on_enter(Engine::ECS::Entity entity)
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

    if (minions_per_summon() <= 0) {
      sm->change_state("Idle");
      return;
    }

    const auto& prefabs = summon_prefabs();
    if (prefabs.empty()) {
      Log::warning("CatMonkey: summon prefabs list is empty; nothing to summon.");
      sm->change_state("Idle");
      return;
    }

    std::vector<Math::Vec2D> spawn_positions = gather_spawn_positions_from_markers(spawn_marker_prefix());
    if (spawn_positions.empty()) {
      // Fallback: spawn around the boss if no markers are present.
      if (auto* t = entity.get_component<Components::Transform2D>()) {
        const float dx = 200.0f;
        spawn_positions.push_back(t->position + Math::Vec2D(-dx, 0.0f));
        spawn_positions.push_back(t->position + Math::Vec2D(dx, 0.0f));
      }
    }

    if (spawn_positions.empty()) {
      Log::warning("CatMonkey: no spawn positions available (markers missing and no Transform2D).");
      sm->change_state("Idle");
      return;
    }

    rt.active_summons.clear();
    rt.active_summons.reserve(static_cast<size_t>(minions_per_summon()));

    for (int i = 0; i < minions_per_summon(); ++i)
    {
      const std::string& prefab_path = prefabs[static_cast<size_t>(i) % prefabs.size()];
      const Math::Vec2D& spawn_pos = spawn_positions[static_cast<size_t>(i) % spawn_positions.size()];

      Engine::ECS::Entity summoned = spawn_enemy_from_prefab(prefab_path, spawn_pos);
      if (summoned.is_valid()) {
        rt.active_summons.push_back(summoned.get_id());
      }
    }

    // Immediately go back to idle to wait for the wave to be cleared.
    sm->change_state("Idle");
  }

  inline void summon_on_update([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // We immediately transition back to Idle in summon_on_enter.
  }

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
  void Cat_monkey_hit_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::hit_on_enter(entity); }
  void Cat_monkey_death_enter(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::death_on_enter(entity); }
  void Cat_monkey_death_update(Engine::ECS::Entity entity) { CatMonkeyStateCallbacks::death_on_update(entity); }
}

REGISTER_FUNCTION(Cat_monkey_idle_enter)
REGISTER_FUNCTION(Cat_monkey_idle_update)
REGISTER_FUNCTION(Cat_monkey_summon_enter)
REGISTER_FUNCTION(Cat_monkey_summon_update)
REGISTER_FUNCTION(Cat_monkey_hit_enter)
REGISTER_FUNCTION(Cat_monkey_death_enter)
REGISTER_FUNCTION(Cat_monkey_death_update)

REGISTER_STATE_MACHINE_BLUEPRINT(CatMonkey, "Idle",
  STATE("Idle", CatMonkeyStateCallbacks::idle_on_enter, CatMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Summon", CatMonkeyStateCallbacks::summon_on_enter, CatMonkeyStateCallbacks::summon_on_update, NO_CALLBACK)
  STATE("Hit", CatMonkeyStateCallbacks::hit_on_enter, NO_CALLBACK, NO_CALLBACK)
  STATE("Death", CatMonkeyStateCallbacks::death_on_enter, CatMonkeyStateCallbacks::death_on_update, NO_CALLBACK)
)

// Optional: explicit registration entrypoint (called from RegisterGameLogic).
// This guarantees the blueprint exists even if static auto-registration is skipped.
void register_catmonkey_blueprint_runtime()
{
  auto& registry = ::Engine::StateMachineBlueprintRegistry::instance();
  if (registry.has_blueprint("CatMonkey")) {
    return;
  }

  ::Engine::StateMachineBlueprint bp;
  bp.initial_state = "Idle";
  STATE("Idle", CatMonkeyStateCallbacks::idle_on_enter, CatMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Summon", CatMonkeyStateCallbacks::summon_on_enter, CatMonkeyStateCallbacks::summon_on_update, NO_CALLBACK)
  STATE("Hit", CatMonkeyStateCallbacks::hit_on_enter, NO_CALLBACK, NO_CALLBACK)
  STATE("Death", CatMonkeyStateCallbacks::death_on_enter, CatMonkeyStateCallbacks::death_on_update, NO_CALLBACK)
  registry.register_blueprint("CatMonkey", bp);
}

//-----------------------------------------------------------------------------------
// file:    CatMonkeyBossComponent.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Editor-exposed component for tuning the Cat Monkey boss.
// 
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

namespace Components {

  struct CatMonkeyBoss
  {
    // Debug/dev toggle: when true, the boss state machine will enter the "Null"
    // state and do nothing (no summoning/attacks). Useful while editing levels.
    bool ai_disabled = false;

    int minions_per_summon = 6;
    // Optional per-wave overrides (0 = use minions_per_summon)
    int summon1_minions = 0;
    int summon2_minions = 0;
    int summon3_minions = 0;
    int summon4_minions = 0;
    int summon5_minions = 0;

    float summon_cooldown = 6.0f;
    // Summon5 special behavior: spawn one minion every N seconds.
    float summon5_spawn_interval_sec = 3.0f;
    int damage_per_minion_death = 1;

    // Optional per-wave spawn set JSON exported from the editor (Selection Export).
    // When set, CatMonkeyStateCallbacks will spawn exactly what is listed in the file
    // (prefab path + position), and will ignore marker/offset-based placement.
    std::string summon1_spawn_set_json{};
    std::string summon2_spawn_set_json{};
    std::string summon3_spawn_set_json{};
    std::string summon4_spawn_set_json{};
    std::string summon5_spawn_set_json{};

    // Optional list of enemy prefab paths to summon for each summon state.
    // If empty, CatMonkeyStateCallbacks will use its built-in defaults.
    std::vector<std::string> summon1_prefabs{};
    std::vector<std::string> summon2_prefabs{};
    std::vector<std::string> summon3_prefabs{};
    std::vector<std::string> summon4_prefabs{};
    std::vector<std::string> summon5_prefabs{};

    // Prefix used to find spawn marker entities by name (chosen locations).
    std::string summon1_spawn_marker_prefix = "cat_monkey_spawn1";
    std::string summon2_spawn_marker_prefix = "cat_monkey_spawn2";
    std::string summon3_spawn_marker_prefix = "cat_monkey_spawn3";
    std::string summon4_spawn_marker_prefix = "cat_monkey_spawn4";
    std::string summon5_spawn_marker_prefix = "cat_monkey_spawn5";

    // Optional spawn offsets (world-space offsets from the boss position).
    // Used when no spawn marker entities are found.
    std::vector<Math::Vec2D> summon1_spawn_offsets{};
    std::vector<Math::Vec2D> summon2_spawn_offsets{};
    std::vector<Math::Vec2D> summon3_spawn_offsets{};
    std::vector<Math::Vec2D> summon4_spawn_offsets{};
    std::vector<Math::Vec2D> summon5_spawn_offsets{};
  };

} // namespace Components

REGISTER_COMPONENT(Components::CatMonkeyBoss,
  ai_disabled,
  minions_per_summon,
  summon1_minions, summon2_minions, summon3_minions, summon4_minions, summon5_minions,
  summon_cooldown, summon5_spawn_interval_sec, damage_per_minion_death,
  summon1_spawn_set_json, summon2_spawn_set_json, summon3_spawn_set_json, summon4_spawn_set_json, summon5_spawn_set_json,
  summon1_prefabs, summon2_prefabs, summon3_prefabs, summon4_prefabs, summon5_prefabs,
  summon1_spawn_marker_prefix, summon2_spawn_marker_prefix, summon3_spawn_marker_prefix, summon4_spawn_marker_prefix, summon5_spawn_marker_prefix,
  summon1_spawn_offsets, summon2_spawn_offsets, summon3_spawn_offsets, summon4_spawn_offsets, summon5_spawn_offsets)

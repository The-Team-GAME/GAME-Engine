//-----------------------------------------------------------------------------------
// file:    CatMonkeyBossComponent.h
// project: Monkey Stole My GPA
//
// brief:   Editor-exposed component for tuning the Cat Monkey boss.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"

namespace Components {

  struct CatMonkeyBoss
  {
    int minions_per_summon = 6;
    float summon_cooldown = 6.0f;
    int damage_per_minion_death = 1;

    // Optional list of enemy prefab paths to summon for each summon state.
    // If empty, CatMonkeyStateCallbacks will use its built-in defaults.
    std::vector<std::string> summon1_prefabs{};
    std::vector<std::string> summon2_prefabs{};
    std::vector<std::string> summon3_prefabs{};

    // Prefix used to find spawn marker entities by name (chosen locations).
    std::string summon1_spawn_marker_prefix = "cat_monkey_spawn1";
    std::string summon2_spawn_marker_prefix = "cat_monkey_spawn2";
    std::string summon3_spawn_marker_prefix = "cat_monkey_spawn3";
  };

} // namespace Components

REGISTER_COMPONENT(Components::CatMonkeyBoss,
  minions_per_summon, summon_cooldown, damage_per_minion_death,
  summon1_prefabs, summon2_prefabs, summon3_prefabs,
  summon1_spawn_marker_prefix, summon2_spawn_marker_prefix, summon3_spawn_marker_prefix)

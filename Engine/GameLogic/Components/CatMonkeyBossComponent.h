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

    // Optional list of enemy prefab paths to summon from.
    // If empty, CatMonkeyStateCallbacks will use its built-in defaults.
    std::vector<std::string> summon_prefabs{};

    // Prefix used to find spawn marker entities by name.
    std::string spawn_marker_prefix = "cat_monkey_spawn";
  };

} // namespace Components

REGISTER_COMPONENT(Components::CatMonkeyBoss,
  minions_per_summon, summon_cooldown, damage_per_minion_death,
  summon_prefabs, spawn_marker_prefix)

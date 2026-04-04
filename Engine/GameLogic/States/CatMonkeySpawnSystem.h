//-----------------------------------------------------------------------------------
// file:    CatMonkeySpawnSystem.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   System and helpers for spawning Cat Monkey boss fight summons. Loads spawn sets
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include <random>
#include "CatMonkeyBossRuntime.h"  // for BossRuntime, Components

namespace CatMonkeyStateCallbacks
{
  // Spawn system declarations
  struct SpawnSetEntry
  {
    std::string prefab_path;
    Math::Vec2D world_pos;
  };

  struct Summon5SpawnItem
  {
    std::string prefab;
    Math::Vec2D pos{ 0.0f, 0.0f };
    bool has_pos = false;
    int max_hp = 0;
  };

  std::vector<SpawnSetEntry> load_spawn_set_entries(Engine::ECS::Entity boss, int summon_index);
  void spawn_wave_for_summon_index(Engine::ECS::Entity entity, int summon_index);
  Engine::ECS::Entity spawn_enemy_from_prefab(const std::string& prefab_path, const Math::Vec2D& pos);
  bool summon5_has_more_to_spawn(const BossRuntime& rt);
  Math::Vec2D summon5_spawn_pos_at(const BossRuntime& rt, size_t index);
  uint32_t summon5_shuffle_seed(Engine::ECS::Entity boss);
  std::vector<Summon5SpawnItem> interleave_items_by_prefab(std::vector<Summon5SpawnItem> items, std::mt19937& rng);
  void randomize_summon5_spawn_order(Engine::ECS::Entity boss, std::vector<std::string>& prefabs, std::vector<Math::Vec2D>* positions, std::vector<int>* max_hps);
}



//-----------------------------------------------------------------------------------
// file:    CheckpointCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//
// email:   o.ziming@digipen.edu
//
// brief:   checkpoint callback
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "./Components/HealthComponents.h"
#include "../../Engine/UpgradeManager.h"

namespace CheckpointCallbacks
{

  Engine::ECS::Entity active_checkpoint{};
  constexpr float CHECKPOINT_HEAL_PER_SEC = 1.0f; // 1 HP per second
  float checkpoint_heal_carry = 0.0f;

  inline void spawn_checkpoint_prompt(Engine::ECS::Entity checkpoint)
  {
    auto* checkpoint_transform = checkpoint.get_component<Components::Transform2D>();
    if (!checkpoint_transform) return;

    auto prefab = Engine::Resources::load_prefab("prefabs/checkpoint_prompt.json");
    Engine::ECS::Entity prompt = Engine::ECS::instantiate_prefab(prefab);

    if (auto* prompt_transform = prompt.get_component<Components::Transform2D>())
    {
      prompt_transform->position = checkpoint_transform->position;
      prompt_transform->position.y -= 50.0f; // above checkpoint
    }

    prompt.add_component<Components::Lifetime>(1.0f);

  }

  void checkpoint_on_enter(Engine::ECS::Entity checkpoint, Engine::ECS::Entity player_entity)
  {
    auto* checkpoint_transform = checkpoint.get_component<Components::Transform2D>();
    auto* checkpoint_data = checkpoint.get_component<Components::Checkpoint>();
    auto* player = player_entity.get_component<Components::Player>();

    if (!checkpoint_transform || !checkpoint_data || !player) return;

    if (checkpoint == active_checkpoint)
      return;

    player->respawn_point = checkpoint_transform->position;
    player->respawn_point.y -= 50.0f; //ground offset
    player->has_checkpoint = true;

    // Snapshot the current upgrade state so dying here restores these upgrades.
    Engine::UpgradeManager::instance().capture_checkpoint_state();

    Log::info("Checkpoint reached at ({}, {})",
      checkpoint_transform->position.x,
      checkpoint_transform->position.y);

    spawn_checkpoint_prompt(checkpoint);

  }

  void checkpoint_on_update([[maybe_unused]] Engine::ECS::Entity checkpoint,
    Engine::ECS::Entity player_entity)
  {

    if (checkpoint == active_checkpoint)
      return;

    auto* health = player_entity.get_component<Components::Health>();
    if (!health) return;

    if (health->value >= health->max)
    {
      checkpoint_heal_carry = 0.0f;
      return;
    }

    checkpoint_heal_carry += CHECKPOINT_HEAL_PER_SEC * Engine::Time::get_fixed_dt();

    while (checkpoint_heal_carry >= 1.0f)
    {
      health->heal(1);
      checkpoint_heal_carry -= 1.0f;

      if (health->value >= health->max)
      {
        checkpoint_heal_carry = 0.0f;
        break;
      }
    }
  }

  void checkpoint_on_exit([[maybe_unused]] Engine::ECS::Entity checkpoint,
    [[maybe_unused]] Engine::ECS::Entity player_entity)
  {
    auto* checkpoint_data = checkpoint.get_component<Components::Checkpoint>();
    if (!checkpoint_data) return;

    if (checkpoint_data->claimed)
      return;

    if (active_checkpoint.is_valid())
    {
      if (auto* old_checkpoint = active_checkpoint.get_component<Components::Checkpoint>())
      {
        old_checkpoint->claimed = false;
      }
    }

    checkpoint_data->claimed = true;
    active_checkpoint = checkpoint;

    checkpoint_heal_carry = 0.0f;
  }
}

REGISTER_FUNCTION(CheckpointCallbacks::checkpoint_on_enter)
REGISTER_FUNCTION(CheckpointCallbacks::checkpoint_on_update)
REGISTER_FUNCTION(CheckpointCallbacks::checkpoint_on_exit)
//-----------------------------------------------------------------------------------
// file:    SpawnpointCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   Spawnpoint callback
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "../Components/GameplayComponent.h"

namespace SpawnpointCallbacks
{


  void spawnpoint_on_enter(Engine::ECS::Entity spawnpoint, Engine::ECS::Entity player_entity)
  {
    auto* spawnpoint_transform = spawnpoint.get_component<Components::Transform2D>();
    auto* player = player_entity.get_component<Components::Player>();

    if (!spawnpoint_transform || !player) return;

    player->respawn_point = spawnpoint_transform->position;
    player->respawn_point.y -= 50.0f; //ground offset
    player->has_checkpoint = true;

    Log::info("Spawnpoint reached at ({}, {})",
      spawnpoint_transform->position.x,
      spawnpoint_transform->position.y);
  }

  void spawnpoint_on_update([[maybe_unused]] Engine::ECS::Entity spawnpoint,
    [[maybe_unused]] Engine::ECS::Entity player_entity)
  {
  }

  void spawnpoint_on_exit([[maybe_unused]] Engine::ECS::Entity spawnpoint,
    [[maybe_unused]] Engine::ECS::Entity player_entity)
  {
  }
}

REGISTER_FUNCTION(SpawnpointCallbacks::spawnpoint_on_enter)
REGISTER_FUNCTION(SpawnpointCallbacks::spawnpoint_on_update)
REGISTER_FUNCTION(SpawnpointCallbacks::spawnpoint_on_exit)
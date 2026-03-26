//-----------------------------------------------------------------------------------
// file:    CatMonkeyStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   State machine callback declarations for Cat Monkey boss fight.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "CatMonkeyBossRuntime.h"
#include "CatMonkeySpawnSystem.h"

namespace CatMonkeyStateCallbacks
{
  // State callbacks forward declarations
  void null_on_enter(Engine::ECS::Entity entity);
  void null_on_update(Engine::ECS::Entity entity);
  void idle_on_enter(Engine::ECS::Entity entity);
  void idle_on_update(Engine::ECS::Entity entity);
  void summon1_on_enter(Engine::ECS::Entity entity);
  void summon1_on_update(Engine::ECS::Entity entity);
  void summon2_on_enter(Engine::ECS::Entity entity);
  void summon2_on_update(Engine::ECS::Entity entity);
  void summon3_on_enter(Engine::ECS::Entity entity);
  void summon3_on_update(Engine::ECS::Entity entity);
  void summon4_on_enter(Engine::ECS::Entity entity);
  void summon4_on_update(Engine::ECS::Entity entity);
  void summon5_on_enter(Engine::ECS::Entity entity);
  void summon5_on_update(Engine::ECS::Entity entity);
  void summon_on_enter(Engine::ECS::Entity entity);
  void summon_on_update(Engine::ECS::Entity entity);
  void hit_on_enter(Engine::ECS::Entity entity);
  void death_on_enter(Engine::ECS::Entity entity);
  void death_on_update(Engine::ECS::Entity entity);

  // Registration
  void register_catmonkey_blueprint_runtime();
}


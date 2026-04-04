//-----------------------------------------------------------------------------------
// file:    TurretMonkeyStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Declarations for Turret Monkey state machine callbacks.
//          Implementation and blueprint registration live in
//          TurretMonkeyStateCallbacks.cpp.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

namespace Engine::ECS {
  class Entity;
}

namespace TurretMonkeyStateCallbacks
{
  void idle_on_enter(Engine::ECS::Entity entity);
  void idle_on_update(Engine::ECS::Entity entity);

  void chasing_on_enter(Engine::ECS::Entity entity);
  void chasing_on_update(Engine::ECS::Entity entity);
  void chasing_on_exit(Engine::ECS::Entity entity);

  void windup_on_enter(Engine::ECS::Entity entity);
  void windup_on_update(Engine::ECS::Entity entity);

  void turret_on_enter(Engine::ECS::Entity entity);
  void turret_on_update(Engine::ECS::Entity entity);
  void turret_on_exit(Engine::ECS::Entity entity);

  void turret_windup_on_enter(Engine::ECS::Entity entity);
  void turret_windup_on_update(Engine::ECS::Entity entity);
  void turret_windup_on_exit(Engine::ECS::Entity entity);

  void turret_attacking_on_enter(Engine::ECS::Entity entity);
  void turret_attacking_on_update(Engine::ECS::Entity entity);
  void turret_attacking_on_exit(Engine::ECS::Entity entity);

  void attacking_on_enter(Engine::ECS::Entity entity);
  void attacking_on_update(Engine::ECS::Entity entity);
  void attacking_on_exit(Engine::ECS::Entity entity);

  void hit_on_enter(Engine::ECS::Entity entity);
  void hit_on_update(Engine::ECS::Entity entity);
  void hit_on_exit(Engine::ECS::Entity entity);

  void death_on_enter(Engine::ECS::Entity entity);
  void death_on_update(Engine::ECS::Entity entity);
  void death_on_exit(Engine::ECS::Entity entity);
}



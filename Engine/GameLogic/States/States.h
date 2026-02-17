//-----------------------------------------------------------------------------------
// file:    States.h
// project: Monkey Stole My GPA
//
// brief:   Unified header for all entity state machine declarations.
//          Callback function declarations and create_state_machine() declarations
//          for Player, LazyMonkey, GamerMonkey, SprayCanMonkey and TurretMonkey.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

// ============================================================================
// Player State Callbacks
// ============================================================================
namespace PlayerStateCallbacks
{
  void take_input(Engine::ECS::Entity entity);
  void idle_on_enter(Engine::ECS::Entity entity);
  void idle_on_update(Engine::ECS::Entity entity);
  void walking_on_enter(Engine::ECS::Entity entity);
  void walking_on_update(Engine::ECS::Entity entity);
  void walking_on_exit(Engine::ECS::Entity entity);
  void jumping_on_enter(Engine::ECS::Entity entity);
  void jumping_on_update(Engine::ECS::Entity entity);
  void jumping_on_exit(Engine::ECS::Entity entity);
  void dash_on_enter(Engine::ECS::Entity entity);
  void dash_on_update(Engine::ECS::Entity entity);
  void dash_on_exit(Engine::ECS::Entity entity);
  void attack1_on_enter(Engine::ECS::Entity entity);
  void attack1_on_update(Engine::ECS::Entity entity);
  void attack2_on_enter(Engine::ECS::Entity entity);
  void attack2_on_update(Engine::ECS::Entity entity);
  void attack3_on_enter(Engine::ECS::Entity entity);
  void attack3_on_update(Engine::ECS::Entity entity);
  void death_on_enter(Engine::ECS::Entity entity);
  void death_on_update(Engine::ECS::Entity entity);
  void death_on_exit(Engine::ECS::Entity entity);
}

namespace PlayerStates
{
  constexpr f32 IDLE_WIDTH = 100.0f;
  constexpr f32 IDLE_HEIGHT = 100.0f;
  constexpr f32 ATTACK_WIDTH = 200.0f;
  constexpr f32 ATTACK_HEIGHT = 100.0f;

  Components::StateMachine create_state_machine();
}

// ============================================================================
// Lazy Monkey State Callbacks
// ============================================================================
namespace LazyMonkeyStateCallbacks
{
  void update_contact_knockback_direction(Engine::ECS::Entity entity);
  void idle_on_enter(Engine::ECS::Entity entity);
  void idle_on_update(Engine::ECS::Entity entity);
  void chasing_on_enter(Engine::ECS::Entity entity);
  void chasing_on_update(Engine::ECS::Entity entity);
  void chasing_on_exit(Engine::ECS::Entity entity);
  void windup_on_enter(Engine::ECS::Entity entity);
  void windup_on_update(Engine::ECS::Entity entity);
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

namespace LazyMonkeyStates
{
  constexpr f32 WIDTH = 45.0f;
  constexpr f32 HEIGHT = 45.0f;
  constexpr f32 ATTACK_WIDTH = 100.0f;
  constexpr f32 ATTACK_HEIGHT = 45.0f;

  Components::StateMachine create_state_machine();
}

// ============================================================================
// Gamer Monkey State Callbacks
// ============================================================================
namespace GamerMonkeyStateCallbacks
{
  void idle_on_enter(Engine::ECS::Entity entity);
  void idle_on_update(Engine::ECS::Entity entity);
  void dash_windup_on_enter(Engine::ECS::Entity entity);
  void dash_windup_on_update(Engine::ECS::Entity entity);
  void dash_windup_on_exit(Engine::ECS::Entity entity);
  void dashing_on_enter(Engine::ECS::Entity entity);
  void dashing_on_update(Engine::ECS::Entity entity);
  void dashing_on_exit(Engine::ECS::Entity entity);
  void chasing_on_enter(Engine::ECS::Entity entity);
  void chasing_on_update(Engine::ECS::Entity entity);
  void chasing_on_exit(Engine::ECS::Entity entity);
  void windup_on_enter(Engine::ECS::Entity entity);
  void windup_on_update(Engine::ECS::Entity entity);
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

namespace GamerMonkeyStates
{
  constexpr f32 WIDTH = 400.0f;
  constexpr f32 HEIGHT = 400.0f;
  constexpr f32 ATTACK_WIDTH = 400.0f;
  constexpr f32 ATTACK_HEIGHT = 400.0f;

  Components::StateMachine create_state_machine();
}

// ============================================================================
// Spray Can Monkey State Callbacks
// ============================================================================
namespace SprayCanMonkey
{
  void update_contact_knockback_direction(Engine::ECS::Entity entity);
  Engine::ECS::Entity spawn_spray_cloud(Engine::ECS::Entity monkey);
  void idle_on_enter(Engine::ECS::Entity entity);
  void idle_on_update(Engine::ECS::Entity entity);
  void chasing_on_enter(Engine::ECS::Entity entity);
  void chasing_on_update(Engine::ECS::Entity entity);
  void chasing_on_exit(Engine::ECS::Entity entity);
  void windup_on_enter(Engine::ECS::Entity entity);
  void windup_on_update(Engine::ECS::Entity entity);
  void spraying_on_enter(Engine::ECS::Entity entity);
  void spraying_on_update(Engine::ECS::Entity entity);
  void spraying_on_exit(Engine::ECS::Entity entity);
  void hit_on_enter(Engine::ECS::Entity entity);
  void hit_on_update(Engine::ECS::Entity entity);
  void hit_on_exit(Engine::ECS::Entity entity);
  void death_on_enter(Engine::ECS::Entity entity);
  void death_on_update(Engine::ECS::Entity entity);
  void death_on_exit(Engine::ECS::Entity entity);
}

namespace SprayCanMonkeyStates
{
  constexpr f32 WIDTH = 80.0f;
  constexpr f32 HEIGHT = 80.0f;
  constexpr f32 SPRAY_WIDTH = 80.0f;

  Components::StateMachine create_state_machine();
}

// ============================================================================
// Turret Monkey State Callbacks
// ============================================================================
namespace TurretMonkeyStateCallbacks
{
  void fire_turret_projectile(Engine::ECS::Entity& entity);
  void fire_spray_cloud(Engine::ECS::Entity& entity);
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

namespace TurretMonkeyStates
{
  constexpr f32 WIDTH = 135.0f;
  constexpr f32 HEIGHT = 135.0f;
  constexpr f32 TURRET_WIDTH = 200.0f;
  constexpr f32 TURRET_HEIGHT = 135.0f;

  Components::StateMachine create_state_machine();
}

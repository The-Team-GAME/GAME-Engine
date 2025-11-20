//-----------------------------------------------------------------------------------
// file:    StateMachine_Example.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Example demonstrating how to use the StateMachine component system.
//          Shows various usage patterns including player character states,
//          enemy AI states, and UI states.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

/*
   USAGE EXAMPLE - Player Character State Machine
   ================================================
*/

#include "Engine/Components/Components.h"
#include "Engine/ECS/ECS.h"
#include "Engine/Input.h"
#include "Engine/Audio.h"
#include "Engine/Resources.h"

void create_player_with_state_machine() {
  using namespace Engine;
  using namespace Components;

  // ========================================================================
  // Step 1: Create the states
  // ========================================================================

  State idle_state(
    "Idle",
    // on_enter
    [](ECS::Entity& entity) {
      auto* rigid = entity.get_component<RigidBody>();
      rigid->velocity.x = 0.f;

      auto* anim = entity.get_component<Animation>();
      anim->change_animation("player_idle");
    },
    // on_update
    [](ECS::Entity& entity) {
      auto* state_machine = entity.get_component<StateMachine>();

      if (Input::is_pressed(KeyCode::W) ||
        Input::is_pressed(KeyCode::A) ||
        Input::is_pressed(KeyCode::S) ||
        Input::is_pressed(KeyCode::D))
      {
        state_machine->change_state("Walking");
      }

      if (Input::is_pressed(KeyCode::F))
      {
        state_machine->change_state("Punching");
      }
    },
    // on_exit
    nullptr // No cleanup needed
  );

  State walking_state(
    "Walking",
    // on_enter
    [](ECS::Entity& entity) {
      auto* anim = entity.get_component<Animation>();
      anim->change_animation("player_walk");
    },
    // on_update
    [](ECS::Entity& entity) {
      auto* rigid = entity.get_component<RigidBody>();
      auto* state_machine = entity.get_component<StateMachine>();

      float speed = 150.f;
      rigid->velocity.x = 0.f;
      rigid->velocity.y = 0.f;

      if (Input::is_pressed(KeyCode::W)) {
        rigid->velocity.y = -speed;
      }
      if (Input::is_pressed(KeyCode::S)) {
        rigid->velocity.y = speed;
      }
      if (Input::is_pressed(KeyCode::A)) {
        rigid->velocity.x = -speed;
      }
      if (Input::is_pressed(KeyCode::D)) {
        rigid->velocity.x = speed;
      }

      if (!Input::is_pressed(KeyCode::W) &&
        !Input::is_pressed(KeyCode::A) &&
        !Input::is_pressed(KeyCode::S) &&
        !Input::is_pressed(KeyCode::D))
      {
        state_machine->change_state("Idle");
      }

      if (Input::is_pressed(KeyCode::F))
      {
        state_machine->change_state("Punching");
      }
    },
    // on_exit
    nullptr
  );

  State punching_state(
    "Punching",
    // on_enter
    [](ECS::Entity& entity) {
      auto* rigid = entity.get_component<RigidBody>();
      rigid->velocity.x = 0.f;

      auto* anim = entity.get_component<Animation>();
      anim->change_animation("player_punch");

      auto punch_sound = Resources::load_sound("sounds/player_punch.wav");
      Audio::play_sound(punch_sound, 0.7f, false);
    },
    // on_update
    [](ECS::Entity& entity) {
      auto* anim = entity.get_component<Animation>();
      auto* state_machine = entity.get_component<StateMachine>();

      if (anim->is_animation_finished())
      {
        state_machine->change_state("Idle");
      }
    },
    // on_exit
    nullptr
  );

  State damage_state(
    "Damage",
    // on_enter
    [](ECS::Entity& entity) {
      auto* rigid = entity.get_component<RigidBody>();
      rigid->velocity.x = 0.f;

      auto* anim = entity.get_component<Animation>();
      anim->change_animation("player_hit");

      auto hit_sound = Resources::load_sound("sounds/player_hit.wav");
      Audio::play_sound(hit_sound, 0.7f, false);

      // Change color to indicate damage
      auto* color = entity.get_component<Color>();
      color->color = Math::Vec4D(1.0f, 0.3f, 0.3f, 1.0f);

      auto* health = entity.get_component<Health>();
      health->current_health -= 10;
    },
    // on_update
    [](ECS::Entity& entity) {
      auto* anim = entity.get_component<Animation>();
      auto* state_machine = entity.get_component<StateMachine>();

      if (anim->is_animation_finished())
      {
        state_machine->change_state("Idle");
      }
    },
    // on_exit
    [](ECS::Entity& entity) {
      // Restore normal color
      auto* color = entity.get_component<Color>();
      color->color = Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f);
    }
  );

  // ========================================================================
  // Step 2: Create the state machine and add states
  // ========================================================================

  StateMachine player_state_machine;
  player_state_machine.add_state(idle_state);
  player_state_machine.add_state(walking_state);
  player_state_machine.add_state(punching_state);
  player_state_machine.add_state(damage_state);
  player_state_machine.set_initial_state("Idle");

  // ========================================================================
  // Step 3: Create the entity with the state machine
  // ========================================================================

  auto player = ECS::create_entity();
  player
    .add_component<Transform2D>(Math::Vec2D(400.f, 300.f))
    .add_component<RigidBody>()
    .add_component<Animation>()
    .add_component<Color>(Math::Vec4D(1.0f, 1.0f, 1.0f, 1.0f))
    .add_component<Health>()
    .add_component<StateMachine>(player_state_machine); // Add the state machine!

  // The StateMachineSystem will automatically update this entity's state machine each frame
}

/*
   USAGE EXAMPLE - Enemy AI State Machine
   =======================================
*/

void create_enemy_with_ai_state_machine() {
  using namespace Engine;
  using namespace Components;

  State patrol_state(
    "Patrol",
    [](ECS::Entity& entity) {
      // on_enter: Start patrolling
      auto* rigid = entity.get_component<RigidBody>();
      rigid->velocity.x = 50.f; // Move right
    },
    [](ECS::Entity& entity) {
      // on_update: Check if player is nearby
      auto* state_machine = entity.get_component<StateMachine>();

      // Simplified distance check (you'd implement this properly)
      bool player_nearby = false; // check_player_distance() < 200.f;

      if (player_nearby) {
        state_machine->change_state("Chase");
      }
    },
    nullptr
  );

  State chase_state(
    "Chase",
    [](ECS::Entity& entity) {
      // on_enter: Start chase animation
      auto* anim = entity.get_component<Animation>();
      anim->change_animation("enemy_run");
    },
    [](ECS::Entity& entity) {
      // on_update: Move towards player
      auto* rigid = entity.get_component<RigidBody>();
      auto* state_machine = entity.get_component<StateMachine>();

      // Move towards player (simplified)
      rigid->velocity.x = 100.f; // move_towards_player();

      // If player is in attack range
      bool in_range = false; // check_attack_range();
      if (in_range) {
        state_machine->change_state("Attack");
      }

      // If player escaped
      bool player_far = true; // check_player_distance() > 300.f;
      if (player_far) {
        state_machine->change_state("Patrol");
      }
    },
    nullptr
  );

  State attack_state(
    "Attack",
    [](ECS::Entity& entity) {
      // on_enter: Play attack animation
      auto* anim = entity.get_component<Animation>();
      anim->change_animation("enemy_attack");

      auto* rigid = entity.get_component<RigidBody>();
      rigid->velocity.x = 0.f; // Stop moving
    },
    [](ECS::Entity& entity) {
      // on_update: Wait for animation to finish
      auto* anim = entity.get_component<Animation>();
      auto* state_machine = entity.get_component<StateMachine>();

      if (anim->is_animation_finished()) {
        state_machine->change_state("Chase");
      }
    },
    nullptr
  );

  StateMachine enemy_ai;
  enemy_ai.add_state(patrol_state);
  enemy_ai.add_state(chase_state);
  enemy_ai.add_state(attack_state);
  enemy_ai.set_initial_state("Patrol");

  auto enemy = ECS::create_entity();
  enemy
    .add_component<Transform2D>(Math::Vec2D(100.f, 300.f))
    .add_component<RigidBody>()
    .add_component<Animation>()
    .add_component<StateMachine>(enemy_ai);
}

/*
   ADVANCED USAGE - Checking State Machine Status
   ==============================================
*/

void advanced_state_machine_usage() {
  using namespace Engine;
  using namespace Components;

  auto entity = ECS::create_entity();
  // ... add components ...

  auto* state_machine = entity.get_component<StateMachine>();

  // Check current state
  if (state_machine->is_in_state("Idle")) {
    // Do something specific for idle state
  }

  // Get current state name
  std::string current = state_machine->get_current_state();

  // Check if first frame of current state
  if (state_machine->is_first_frame()) {
    // One-time initialization that wasn't in on_enter
  }

  // Revert to previous state
  state_machine->revert_to_previous_state();

  // Force immediate transition (use with caution!)
  state_machine->force_immediate_state_change("EmergencyState");

  // Check state history (for debugging)
  auto& history = state_machine->get_state_history();
  for (const auto& state_name : history) {
    // Log or inspect previous states
  }
}

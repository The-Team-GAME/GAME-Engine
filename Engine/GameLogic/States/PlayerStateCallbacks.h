//-----------------------------------------------------------------------------------
// file:    PlayerStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (50%)
//          Primary author
//          Cheng Ming Xian (50%)
//          Co-Author
// 
// email:   chew.s@digipen.edu
//          c.mingxian@digipen.edu
//
// brief:   Player state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"
#include <assert.h>
namespace {
  f32 timer;
  f32 dash_cooldown;
  f32 terminal_vel_placeholder;
}
namespace PlayerStateCallbacks
{
  // ========================================================================
  // Taking Updates in to handle the states
  // ========================================================================
  void take_input(ECS::Entity entity)
  {
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::RigidBody* player_rigid = entity.get_component<Components::RigidBody>();
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::Flip* flip = entity.get_component<Components::Flip>();

    // Handling cooldowns
    if (dash_cooldown > 0) { dash_cooldown -= Time::get_fixed_dt(); }

    // Check for attack input
    if (Engine::Input::is_pressed(Engine::KeyCode::J))
    {
      state_machine->change_state("Attack1");
      return;
    }

    // Check for movement input
    if (Input::is_held(KeyCode::ShiftLeft) && dash_cooldown <= 0.f)
    {
      state_machine->change_state("Dash");
      return;
    }

    if (Input::is_held(KeyCode::Space)
      && player_rigid->on_ground
      && !player_rigid->is_jumping)
    {
      state_machine->change_state("Jumping");
      return;
    }

    if (Input::is_held(KeyCode::D) || Input::is_held(KeyCode::A))
    {
      if (Input::is_held(KeyCode::A)) //left
      {
        flip->value.x = -1.f;
      }
      else if (Input::is_held(KeyCode::D)) //right
      {
        flip->value.x = 1.f;
      }
      player_rigid->force.x += player_comp->walk_speed * flip->value.x;
      if (state_machine->get_current_state() != "Jumping")
      {
        state_machine->change_state("Walking");
      }
      return;
    }

    // If no valid inputs
    if (state_machine->get_current_state() != "Jumping")
    {
      state_machine->change_state("Idle");
    }

  }

#pragma region idle state;
  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    /* Empty By Design */
    Log::info("Player: Entered Idle state");
  }

  inline void idle_on_update(Engine::ECS::Entity entity) {
    take_input(entity);
  }
#pragma endregion

#pragma region Walking State
  // ========================================================================
  // Walking State Callbacks
  // ========================================================================

  inline void walking_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    Log::info("Player: Entered Walking state");

  }

  inline void walking_on_update(Engine::ECS::Entity entity)
  {
    // we set an exit criteria for the walking state instead of reseting it every frame as the animation sequence needs the state to be constant for this to work
    if (!Input::is_held(KeyCode::A) && !Input::is_held(KeyCode::D))
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      state_machine->change_state("Idle");
    }
    take_input(entity);
  }

  inline void walking_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // Empty by design
  }
#pragma endregion

#pragma region Jumping State
  // ========================================================================
  // Jumping State Callbacks
  // ========================================================================

  inline void jumping_on_enter(Engine::ECS::Entity entity)
  {
    Log::info("Player: Entered Jumping state");
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(rigid);
    assert(player_comp);
    if (player_comp)
    {
      rigid->is_jumping = true;
      rigid->on_ground = false;
      rigid->force.y += player_comp->jump_force;
    }
  }

  inline void jumping_on_update(Engine::ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    assert(rigid);
    assert(player_comp);
    assert(state_machine);

    if (rigid->on_ground && rigid->velocity.y >= 0.0f)
    {
      state_machine->change_state("Idle");
    }
    take_input(entity);
  }

  inline void jumping_on_exit(Engine::ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->is_jumping = false;
    Log::info("Player: Exiting Jumping state");
  }
#pragma endregion

#pragma region Dash
  // ========================================================================
  // Dash State Callbacks
  // ========================================================================

  inline void dash_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Dashing state");

    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->dash_timer = player_comp->dash_duration;

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->use_gravity = false;
    rigid->velocity.zero();
    rigid->force.zero();
    terminal_vel_placeholder = rigid->x_terminal_velocity;
  }

  inline void dash_on_update(ECS::Entity entity)
  {

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(rigid);
    assert(player_comp);
    assert(flip);
    if (player_comp->dash_timer <= 0.f)
    {
      player_comp->dash_timer = 0.f;
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("Idle");
    }
    rigid->x_terminal_velocity = player_comp->dash_speed;
    rigid->velocity.x = flip->value.x * player_comp->dash_speed;
    player_comp->dash_timer -= Time::get_fixed_dt();
  }

  inline void dash_on_exit(ECS::Entity entity)
  {
    Log::info("Player: Exited Dashing state");
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->use_gravity = true;
    rigid->x_terminal_velocity = terminal_vel_placeholder;
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    dash_cooldown = player_comp->dash_cooldown;
  }
#pragma endregion

#pragma region attack 1
  // ========================================================================
  // Attack 1 State Callbacks (First Punch)
  // ========================================================================

  inline void attack1_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player_comp = entity.get_component<Components::Player>();
    auto* heartrate = entity.get_component<Components::Heartrate>();

    // Increase heartrate
    if (heartrate) {
      heartrate->increase(player_comp->attack1.heartrate_cost);
    }

    // Apply forward movement during attack
    if (rigid && flip && player_comp) {
      rigid->velocity.x = (flip->value.x < 0) ? -player_comp->attack1.dash_speed : player_comp->attack1.dash_speed;
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();

    if (transform && player_comp) {
      float offset_x = (flip && flip->value.x < 0) ? -player_comp->attack1.hitbox_offset_x : player_comp->attack1.hitbox_offset_x;
      float offset_y = player_comp->attack1.hitbox_offset_y;

      auto hitbox = Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Attack>(player_comp->attack1.damage)
        .add_component<Components::Transform2D>(
          transform->position.x + offset_x,
          transform->position.y + offset_y,
          0.0f,
          player_comp->attack1.hitbox_width,
          player_comp->attack1.hitbox_height
        )
        .add_component<Components::RigidBody>(true, false)
        .add_component<Components::BoxCollider>(player_comp->attack1.hitbox_width, player_comp->attack1.hitbox_height, false)
        .add_component<Components::Parent>(entity.get_id())
        .add_component<Components::Lifetime>(0.48f) // Attack1 duration: 6 frames × 0.08f
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");

      // Apply knockback direction based on facing
      auto* attack = hitbox.get_component<Components::Attack>();
      if (attack) {
        attack->knockback = player_comp->attack1.knockback;
        if (flip && flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);

        // Assign impact VFX texture
        attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
      }
    }

    Log::info("Player: Attack 1 (First Punch)");
  }

  inline void attack1_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    if (!animation) return;

    // Check if player pressed attack again during this combo window
    if (Engine::Input::is_pressed(Engine::KeyCode::J) && !animation->has_ended()) {
      // Queue up next attack
      if (animation->get_current_frame_index() >= 3) { // Allow combo after frame 3
        state_machine->change_state("Attack2");
      }
    }

    // Transition to idle when attack animation completes
    if (animation->has_ended()) {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      // offset slightly forward because attack texture is wider than idle texture
      if (transform && flip) {
        transform->position.x += (flip->value.x < 0) ? 20.0f : -20.0f;
      }

      // Force so that we don't display the attack texture after offset is applied
      state_machine->change_state("Idle");
    }
  }
#pragma endregion

#pragma region attack 2
  // ========================================================================
  // Attack 2 State Callbacks (Second Punch)
  // ========================================================================

  inline void attack2_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player_comp = entity.get_component<Components::Player>();
    auto* heartrate = entity.get_component<Components::Heartrate>();

    // Increase heartrate
    if (heartrate) {
      heartrate->increase(player_comp->attack2.heartrate_cost);
    }

    // Apply forward movement during attack
    if (rigid && flip && player_comp) {
      rigid->velocity.x = (flip->value.x < 0) ? -player_comp->attack2.dash_speed : player_comp->attack2.dash_speed;
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();

    if (transform && player_comp) {
      float offset_x = (flip && flip->value.x < 0) ? -player_comp->attack2.hitbox_offset_x : player_comp->attack2.hitbox_offset_x;
      float offset_y = player_comp->attack2.hitbox_offset_y;

      auto hitbox = Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Attack>(player_comp->attack2.damage)
        .add_component<Components::Transform2D>(
          transform->position.x + offset_x,
          transform->position.y + offset_y,
          0.0f,
          player_comp->attack2.hitbox_width,
          player_comp->attack2.hitbox_height
        )
        .add_component<Components::RigidBody>(true, false)
        .add_component<Components::BoxCollider>(player_comp->attack2.hitbox_width, player_comp->attack2.hitbox_height, false)
        .add_component<Components::Parent>(entity.get_id())
        .add_component<Components::Lifetime>(0.24f) // Attack2 duration: 3 frames × 0.08f
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");

      // Apply knockback direction based on facing
      auto* attack = hitbox.get_component<Components::Attack>();
      if (attack) {
        attack->knockback = player_comp->attack2.knockback;
        if (flip && flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);

        // Assign impact VFX texture
        attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBluePink_Spritesheet.png");
      }
    }

    Log::info("Player: Attack 2 (Second Punch)");
  }

  inline void attack2_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    if (!animation) return;

    // Check if player pressed attack again during this combo window
    if (Engine::Input::is_pressed(Engine::KeyCode::J) && !animation->has_ended()) {
      // Allow combo after frame 1 of this attack
      if (animation->get_current_frame_index() >= 1) {
        state_machine->change_state("Attack3");
      }
    }

    // Transition to idle when attack animation completes
    if (animation->has_ended()) {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      // offset slightly forward because attack texture is wider than idle texture
      if (transform && flip) {
        transform->position.x += (flip->value.x < 0) ? 20.0f : -20.0f;
      }

      // Force so that we don't display the attack texture after offset is applied
      state_machine->change_state("Idle");
    }
  }
#pragma endregion

#pragma region attack 3
  // ========================================================================
  // Attack 3 State Callbacks (Third Punch - Finisher)
  // ========================================================================

  inline void attack3_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* player_comp = entity.get_component<Components::Player>();
    auto* heartrate = entity.get_component<Components::Heartrate>();

    // Increase heartrate
    if (heartrate) {
      heartrate->increase(player_comp->attack3.heartrate_cost);
    }

    // Apply forward movement during attack
    if (rigid && flip && player_comp) {
      rigid->velocity.x = (flip->value.x < 0) ? -player_comp->attack3.dash_speed : player_comp->attack3.dash_speed;
    }

    // Create attack hitbox entity
    auto* transform = entity.get_component<Components::Transform2D>();

    if (transform && player_comp) {
      float offset_x = (flip && flip->value.x < 0) ? -player_comp->attack3.hitbox_offset_x : player_comp->attack3.hitbox_offset_x;
      float offset_y = player_comp->attack3.hitbox_offset_y;

      auto hitbox = Engine::ECS::create_entity()
        .add_component<Components::Tag<"PlayerAttackHitbox">>()
        .add_component<Components::Attack>(player_comp->attack3.damage)
        .add_component<Components::Transform2D>(
          transform->position.x + offset_x,
          transform->position.y + offset_y,
          0.0f,
          player_comp->attack3.hitbox_width,
          player_comp->attack3.hitbox_height
        )
        .add_component<Components::RigidBody>(true, false)
        .add_component<Components::BoxCollider>(player_comp->attack3.hitbox_width, player_comp->attack3.hitbox_height, false)
        .add_component<Components::Parent>(entity.get_id())
        .add_component<Components::Lifetime>(0.6f) // Attack3 duration: 10 frames × 0.06f
        .remove_from_layer("default")
        .add_to_layer("damage_enemy");

      // Apply knockback direction based on facing
      auto* attack = hitbox.get_component<Components::Attack>();
      if (attack) {
        attack->knockback = player_comp->attack3.knockback;
        if (flip && flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);

        // Assign impact VFX texture
        attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactPink_Spritesheet.png");
      }
    }

    Log::info("Player: Attack 3 (Third Punch - Finisher!)");
  }

  inline void attack3_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // No combo after this - it's the finisher
    // Just wait for animation to complete
    if (animation && animation->has_ended()) {
      auto* transform = entity.get_component<Components::Transform2D>();
      auto* flip = entity.get_component<Components::Flip>();
      // offset slightly forward because attack texture is wider than idle texture
      if (transform && flip) {
        transform->position.x += (flip->value.x < 0) ? 20.0f : -20.0f;
      }

      // Force so that we don't display the attack texture after offset is applied
      state_machine->change_state("Idle");
    }
  }
#pragma endregion
} // namespace PlayerStateCallbacks

// Register all player state callbacks
REGISTER_FUNCTION(player_idle_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::idle_on_enter)
REGISTER_FUNCTION(player_idle_update, void(Engine::ECS::Entity), PlayerStateCallbacks::idle_on_update)

REGISTER_FUNCTION(player_walking_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::walking_on_enter)
REGISTER_FUNCTION(player_walking_update, void(Engine::ECS::Entity), PlayerStateCallbacks::walking_on_update)
REGISTER_FUNCTION(player_walking_exit, void(Engine::ECS::Entity), PlayerStateCallbacks::walking_on_exit)

REGISTER_FUNCTION(player_jumping_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::jumping_on_enter)
REGISTER_FUNCTION(player_jumping_update, void(Engine::ECS::Entity), PlayerStateCallbacks::jumping_on_update)
REGISTER_FUNCTION(player_jumping_exit, void(Engine::ECS::Entity), PlayerStateCallbacks::jumping_on_exit)

REGISTER_FUNCTION(player_dash_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::dash_on_enter)
REGISTER_FUNCTION(player_dash_update, void(Engine::ECS::Entity), PlayerStateCallbacks::dash_on_update)
REGISTER_FUNCTION(player_dash_exit, void(Engine::ECS::Entity), PlayerStateCallbacks::dash_on_exit)

REGISTER_FUNCTION(player_attack1_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::attack1_on_enter)
REGISTER_FUNCTION(player_attack1_update, void(Engine::ECS::Entity), PlayerStateCallbacks::attack1_on_update)

REGISTER_FUNCTION(player_attack2_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::attack2_on_enter)
REGISTER_FUNCTION(player_attack2_update, void(Engine::ECS::Entity), PlayerStateCallbacks::attack2_on_update)

REGISTER_FUNCTION(player_attack3_enter, void(Engine::ECS::Entity), PlayerStateCallbacks::attack3_on_enter)
REGISTER_FUNCTION(player_attack3_update, void(Engine::ECS::Entity), PlayerStateCallbacks::attack3_on_update)

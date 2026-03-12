//-----------------------------------------------------------------------------------
// file:    PlayerStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (80%)
//          Primary author
//          Chew Shee Yang (20%)
//          Co-Author
// 
// email:   c.mingxian@digipen.edu
//          chew.s@digipen.edu
//
// brief:   Player state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"
#include "../Components/HealthComponents.h"

using namespace Engine;
namespace {
  f32 terminal_vel_placeholder;
  // This bool default_states is for when we want the player still be able to give inputs (and possibly exit the state) but still keep the animation of the state
  bool default_state = false;
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
    Components::Health* health = entity.get_component<Components::Health>();

    if (!state_machine || !player_rigid || !player_comp || !flip || !health)
    {
      Log::error("PlayerStateCallbacks: Missing required components on entity {}", entity.get_id());
      return;
    }

    // Handling cooldowns
    if (!player_comp->dash.ready())
    {
      player_comp->dash.cd_timer -= Time::get_fixed_dt();
    }
    if (!player_comp->uppercut.ready())
    {
      player_comp->uppercut.cd_timer -= Time::get_fixed_dt();
    }
    if (!player_comp->smash.ready())
    {
      player_comp->smash.cd_timer -= Time::get_fixed_dt();
    }

#pragma region Atk Input
    if (Engine::Input::is_pressed(Engine::KeyCode::K)
      && player_comp->uppercut.ready())
    {
      // putting this here because it spazes otherwise, i think its smth to do with drawing sequence or smth
      Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
      assert(xform);
      xform->position.y -= 32.f;

      state_machine->change_state("PreUppercut");
      return;
    }

    if (Engine::Input::is_pressed(Engine::KeyCode::L)
      && player_comp->smash.ready())
    {
      if (player_rigid->on_ground)
      {
        state_machine->change_state("Leap");
      }
      else
      {
        state_machine->change_state("PreSmash");
      }
      return;
    }

    if (Engine::Input::is_pressed(Engine::KeyCode::J))
    {
      state_machine->change_state("Attack1");
      return;
    }
#pragma endregion

#pragma region Movement Input
    if (Input::is_held(KeyCode::ShiftLeft)
      && player_comp->dash.ready())
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

    // When A and D are pressed
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

      player_rigid->force.x = player_comp->walk_speed * flip->value.x;
      // This part to deal with walking on slopes (WIP)
      //if (player_rigid->on_ground)
      //{
      //  // we need to check if the direction vector of the slope is the same as the flip value
      //  Vec2D direction = player_rigid->last_surface_dir_vector;
      //  if (dot_product(direction, Vec2D(flip->value.x, 0.f)) < 0)
      //  {
      //    direction *= -1;
      //  }
      //  std::cout << direction << std::endl;
      //  player_rigid->force = direction * player_comp->walk_speed;
      //}
      //else
      //{
      //  player_rigid->force.x = player_comp->walk_speed * flip->value.x;
      //}

      if (!default_state)
      {
        state_machine->change_state("Walking");
      }
      return;
    }
#pragma endregion
    // If no valid inputs
    if (!default_state)
    {
      state_machine->change_state("Idle");
    }
  }

  // Takes in the parent entity as well as the attack data of the attack to generate a hitbox
  ECS::Entity create_hitbox(ECS::Entity parent, Components::AttackData& atk_data)
  {
    Components::Transform2D* xform = parent.get_component<Components::Transform2D>();
    Components::Flip* flip = parent.get_component<Components::Flip>();
    assert(xform);
    assert(flip);
    ECS::Entity hitbox = Engine::ECS::create_entity()
      .add_component<Components::Attack>(atk_data.dmg)
      .add_component<Components::Transform2D>( 
        xform->position.x + flip->value.x * atk_data.hitbox_offset_x,
        xform->position.y + atk_data.hitbox_offset_y,
        0.0f, atk_data.hitbox_width, atk_data.hitbox_height )
      .add_component<Components::RigidBody>(true, false)
      .add_component<Components::BoxCollider>(atk_data.hitbox_width, atk_data.hitbox_height, false)
      .add_component<Components::Parent>(parent.get_id())
      .add_component<Components::Lifetime>(atk_data.life_time) // Attack1 duration: 6 frames × 0.08f
      .remove_from_layer("default")
      .add_to_layer("damage_enemy");

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->knockback = atk_data.knockback * flip->value.x;
    // Knockback Y is always positive (up) in our config, but physics needs negative for up
    attack->knockback.y = -std::abs(attack->knockback.y);
    return hitbox;
  }

#pragma region idle state;
  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
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
    default_state = true;
  }

  inline void walking_on_update(Engine::ECS::Entity entity)
  {
    // we set an exit criteria for the walking state instead of reseting it every frame as the animation sequence needs the state to be constant for this to work
    Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
    assert(sm);
    if (!Input::is_held(KeyCode::A) && !Input::is_held(KeyCode::D))
    {
      sm->change_state("Idle");
    }
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    if (!rigid->on_ground)
    {
      sm->change_state("Falling");
    }
    take_input(entity);
  }

  inline void walking_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
    default_state = false;
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
    rigid->is_jumping = true;
    rigid->on_ground = false;

    rigid->velocity.zero();
    rigid->force.zero();
    rigid->force.y += player_comp->jump_force;

    default_state = true;
  }

  inline void jumping_on_update(Engine::ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);

    // we check for rigid->on_ground here because on the first few frames there is still a chance that the player will be counted as on gound
    if (rigid->on_ground)// && rigid->velocity.y >= 0.0f)
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Falling");
    }
    take_input(entity);
  }

  inline void jumping_on_exit(Engine::ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->is_jumping = false;
    default_state = false;
  }
#pragma endregion

#pragma region Falling
  // ========================================================================
  // Falling State Callbacks
  // ========================================================================

  inline void falling_on_enter([[maybe_unused]] ECS::Entity entity)
  {
    Log::info("Player: Entered Falling state");
    default_state = true;
  }

  inline void falling_on_update(ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    if (rigid->on_ground)
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Idle");
    }
    take_input(entity);
  }

  inline void falling_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    default_state = false;
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
    player_comp->dash.set_timer();
    player_comp->damage_immune = true;

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(flip);

    rigid->apply_force = false;
    rigid->velocity.y = 0.f;
    rigid->velocity.x = flip->value.x * player_comp->dash.speed;
    terminal_vel_placeholder = rigid->x_terminal_velocity;
    rigid->x_terminal_velocity = player_comp->dash.speed;
  }

  inline void dash_on_update(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    if (player_comp->dash.timer_end())
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
      assert(sm);
      assert(rigid);
      if (rigid->on_ground)
      {
        sm->change_state("Idle");
      }
      else
      {
        sm->change_state("Falling");
      }
    }
    player_comp->dash.timer -= Time::get_fixed_dt();
  }

  inline void dash_on_exit(ECS::Entity entity)
  {
    Log::info("Player: Exited Dashing state");
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->apply_force = true;
    rigid->x_terminal_velocity = terminal_vel_placeholder;
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->dash.reset_cooldown();
    player_comp->damage_immune = false;
  }
#pragma endregion

#pragma region Stunned
  // ========================================================================
  // Stunned State Callbacks
  // ========================================================================

  inline void stunned_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Stunned!");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->damage_immune = true;
    player_comp->stunned.set_timer();
  }

  inline void stunned_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended()) 
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Idle");
    }
  }

  inline void stunned_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    Log::info("Player: Exited Stunned state");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    player_comp->damage_immune = false;
  }
#pragma endregion

#pragma region attack 1
  // ========================================================================
  // Attack 1 State Callbacks (First Punch)
  // ========================================================================

  inline void attack1_on_enter(Engine::ECS::Entity entity) {
    Log::info("Player: Attack 1 (First Punch)");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(player_comp);
    assert(rigid);
    assert(flip);

    player_comp->knockback_value = player_comp->attack1_data.knockback;
    rigid->velocity.x = flip->value.x * player_comp->dash.speed;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->attack1_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void attack1_on_update(Engine::ECS::Entity entity) {
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(state_machine);
    assert(animation);

    // Check if player pressed attack again during this combo window
    if (Engine::Input::is_pressed(Engine::KeyCode::J) && !animation->has_ended()) {
      // Queue up next attack
      if (animation->get_current_frame_index() >= 3) { // Allow combo after frame 3
        state_machine->change_state("Attack2");
      }
    }
    // Transition to idle when attack animation completes
    if (animation->has_ended()) 
    {
      state_machine->change_state("Idle");
    }
  }
#pragma endregion

#pragma region attack 2
  // ========================================================================
  // Attack 2 State Callbacks (Second Punch)
  // ========================================================================

  inline void attack2_on_enter(Engine::ECS::Entity entity) {
    Log::info("Player: Attack 2 (Second Punch)");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(player_comp);
    assert(rigid);
    assert(flip);

    player_comp->knockback_value = player_comp->attack2_data.knockback;
    rigid->velocity.x = flip->value.x * player_comp->dash.speed;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->attack2_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void attack2_on_update(Engine::ECS::Entity entity) {
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(state_machine);
    assert(animation);

    // Check if player pressed attack again during this combo window
    if (Engine::Input::is_pressed(Engine::KeyCode::J) && !animation->has_ended()) {
      // Allow combo after frame 1 of this attack
      if (animation->get_current_frame_index() >= 1) {
        state_machine->change_state("Attack3");
      }
    }
    // Transition to idle when attack animation completes
    if (animation->has_ended()) 
    {
      state_machine->change_state("Idle");
    }
  }
#pragma endregion

#pragma region attack 3
  // ========================================================================
  // Attack 3 State Callbacks (Third Punch - Finisher)
  // ========================================================================

  inline void attack3_on_enter(Engine::ECS::Entity entity) {
    Log::info("Player: Attack 3 (Third Punch - Finisher!)");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(player_comp);
    assert(rigid);
    assert(flip);

    player_comp->knockback_value = player_comp->attack3_data.knockback;
    rigid->velocity.x = flip->value.x * player_comp->dash.speed;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->attack3_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void attack3_on_update(Engine::ECS::Entity entity) {
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(state_machine);
    assert(animation);

    // No combo after this - it's the finisher
    // Just wait for animation to complete
    if (animation->has_ended()) 
    {
      state_machine->change_state("Idle");
    }
  }
#pragma endregion

#pragma region uppercut
  // ========================================================================
  // Pre Uppercut State Callbacks
  // ========================================================================

  inline void pre_uppercut_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Pre Uppercut state");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->knockback_value = player_comp->uppercut_data.knockback;
    player_comp->uppercut.reset_cooldown();
    player_comp->damage_immune = true;

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->apply_force = false;
    rigid->velocity.zero();

    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->uppercut_data);

    //Components::Attack* attack = hitbox.get_component<Components::Attack>();
    //assert(attack);
    //attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void pre_uppercut_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("Uppercut");
    }
  }

  inline void pre_uppercut_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    // Empty as designed
  }

  // ========================================================================
  // Uppercut State Callbacks
  // ========================================================================

  inline void uppercut_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Uppercut state");

    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->uppercut.set_timer();

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    terminal_vel_placeholder = rigid->y_terminal_velocity;
    rigid->y_terminal_velocity = player_comp->uppercut.speed;
    rigid->velocity = Vec2D{ 0, -player_comp->uppercut.speed };
  }

  inline void uppercut_on_update(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);

    if (player_comp->uppercut.timer_end())
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("PostUppercut");
    }

    player_comp->uppercut.timer -= Time::get_fixed_dt();
  }

  inline void uppercut_on_exit(ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->y_terminal_velocity = terminal_vel_placeholder;
  }

  // ========================================================================
  // Post Uppercut State Callbacks (Used to complete the uppercut animation)
  // ========================================================================
  // possibly the hangtime part of the state as well
  inline void post_uppercut_on_enter([[maybe_unused]] ECS::Entity entity)
  {
    Log::info("Player: Entered Post Uppercut state");

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->apply_force = false;
    rigid->velocity = rigid->velocity * 0.05f;
  }

  inline void post_uppercut_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("UppercutFalling");
    }
  }

  inline void post_uppercut_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->apply_force = true; // renabling gravity

    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->damage_immune = false;
  }

  // ========================================================================
  // Uppercut Falling State Callbacks
  // ========================================================================
  // The falling part of the uppercut
  inline void uppercut_falling_on_enter([[maybe_unused]] ECS::Entity entity)
  {
    Log::info("Player: Entered Post Uppercut state");
    default_state = true;
  }

  inline void uppercut_falling_on_update(ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    if (rigid->on_ground)
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Idle");
    }
    take_input(entity);
  }

  inline void uppercut_falling_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    default_state = false;
    Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
    assert(xform);
    xform->position.y += 32.f;
  }
#pragma endregion

#pragma region smash
  // ========================================================================
  // Leap into Smash State Callbacks
  // ========================================================================

  inline void leap_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Pre Smash state");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->damage_immune = true;
    player_comp->leap.set_timer();

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(rigid);
    rigid->velocity = Vec2D{ flip->value.x * player_comp->leap.speed,
                             -player_comp->leap.speed };
  }

  inline void leap_on_update(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    if (player_comp->leap.timer_end())
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("PreSmash");
    }
    player_comp->leap.timer -= Time::get_fixed_dt();
  }

  inline void leap_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    // Empty as designed
  }

  // ========================================================================
  // Pre Smash State Callbacks
  // ========================================================================

  inline void pre_smash_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Pre Smash state");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->smash.reset_cooldown();
    player_comp->damage_immune = true;

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->apply_force = false;
    rigid->velocity = rigid->velocity * 0.05f;
  }

  inline void pre_smash_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("Smash");
    }
  }

  inline void pre_smash_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    // Empty as designed
  }

  // ========================================================================
  // Smash State Callbacks
  // ========================================================================

  inline void smash_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Smash state");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    terminal_vel_placeholder = rigid->y_terminal_velocity;
    rigid->velocity = Vec2D{ 0, player_comp->smash.speed };
  }

  inline void smash_on_update(ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    if (rigid->on_ground)
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("PostSmash");
    }
  }

  inline void smash_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    // Empty as designed
  }

  // ========================================================================
  // Post Smash State Callbacks
  // ========================================================================

  inline void post_smash_on_enter(ECS::Entity entity)
  {
    Log::info("Player: Entered Post Smash state");
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);

    player_comp->knockback_value = player_comp->smash_data.knockback;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->smash_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    if (attack) {
      attack->play_punch_hit_sfx = false;
    }

    /*Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");*/
  }

  inline void post_smash_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended())
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("Idle");
    }
  }

  inline void post_smash_on_exit(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);
    player_comp->damage_immune = false;

    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->y_terminal_velocity = terminal_vel_placeholder;
    rigid->apply_force = true;
  }
#pragma endregion

#pragma region Death
  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  inline void death_on_enter([[maybe_unused]] ECS::Entity entity)
  {
    Log::info("Player: Died!");
  }

  inline void death_on_update(ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended()) {
      // Engine::ECS::destroy_entity(entity);
      Log::info("Player: Destroyed after death animation.");
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("Idle");
    }
  }

  inline void death_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    Log::info("Player: Exited Death state");
    //Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    //assert(state_machine);
    //state_machine->change_state("Idle");

    //std::string scene = SceneManager::get_current_scene_name();
    //SceneManager::load_scene(scene);

    auto* player = entity.get_component<Components::Player>();

    if (!player->has_checkpoint) {
      std::string scene = SceneManager::get_current_scene_name();
      SceneManager::load_scene(scene);
    }

    auto* transform = entity.get_component<Components::Transform2D>();
    auto* rigidbody = entity.get_component<Components::RigidBody>();
    auto* health = entity.get_component<Components::Health>();
    auto* state_machine = entity.get_component<Components::StateMachine>();

    if (!player || !transform || !rigidbody || !health || !state_machine) return;

    if (player->has_checkpoint)
      transform->position = player->respawn_point;

    rigidbody->velocity = { 0.0f, 0.0f };
    rigidbody->acceleration = { 0.0f, 0.0f };

    health->value = health->max;

    state_machine->change_state("Idle");
  }
#pragma endregion

} // namespace PlayerStateCallbacks

// Register blueprint + all player state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(Player, "Idle",
  STATE("Idle", PlayerStateCallbacks::idle_on_enter, PlayerStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Walking", PlayerStateCallbacks::walking_on_enter, PlayerStateCallbacks::walking_on_update, PlayerStateCallbacks::walking_on_exit)
  STATE("Jumping", PlayerStateCallbacks::jumping_on_enter, PlayerStateCallbacks::jumping_on_update, PlayerStateCallbacks::jumping_on_exit)
  STATE("Falling", PlayerStateCallbacks::falling_on_enter, PlayerStateCallbacks::falling_on_update, PlayerStateCallbacks::falling_on_exit)
  STATE("Dash", PlayerStateCallbacks::dash_on_enter, PlayerStateCallbacks::dash_on_update, PlayerStateCallbacks::dash_on_exit)

  STATE("Attack1", PlayerStateCallbacks::attack1_on_enter, PlayerStateCallbacks::attack1_on_update, NO_CALLBACK)
  STATE("Attack2", PlayerStateCallbacks::attack2_on_enter, PlayerStateCallbacks::attack2_on_update, NO_CALLBACK)
  STATE("Attack3", PlayerStateCallbacks::attack3_on_enter, PlayerStateCallbacks::attack3_on_update, NO_CALLBACK)

  STATE("PreUppercut", PlayerStateCallbacks::pre_uppercut_on_enter, PlayerStateCallbacks::pre_uppercut_on_update, PlayerStateCallbacks::pre_uppercut_on_exit)
  STATE("Uppercut", PlayerStateCallbacks::uppercut_on_enter, PlayerStateCallbacks::uppercut_on_update, PlayerStateCallbacks::uppercut_on_exit)
  STATE("PostUppercut", PlayerStateCallbacks::post_uppercut_on_enter, PlayerStateCallbacks::post_uppercut_on_update, PlayerStateCallbacks::post_uppercut_on_exit)
  STATE("UppercutFalling", PlayerStateCallbacks::uppercut_falling_on_enter, PlayerStateCallbacks::uppercut_falling_on_update, PlayerStateCallbacks::uppercut_falling_on_exit)
  
  STATE("Leap", PlayerStateCallbacks::leap_on_enter, PlayerStateCallbacks::leap_on_update, PlayerStateCallbacks::leap_on_exit)
  STATE("PreSmash", PlayerStateCallbacks::pre_smash_on_enter, PlayerStateCallbacks::pre_smash_on_update, PlayerStateCallbacks::pre_smash_on_exit)
  STATE("Smash", PlayerStateCallbacks::smash_on_enter, PlayerStateCallbacks::smash_on_update, PlayerStateCallbacks::smash_on_exit)
  STATE("PostSmash", PlayerStateCallbacks::post_smash_on_enter, PlayerStateCallbacks::post_smash_on_update, PlayerStateCallbacks::post_smash_on_exit)

  STATE("Stunned", PlayerStateCallbacks::stunned_on_enter, PlayerStateCallbacks::stunned_on_update, PlayerStateCallbacks::stunned_on_exit)
  STATE("Death", PlayerStateCallbacks::death_on_enter, PlayerStateCallbacks::death_on_update, PlayerStateCallbacks::death_on_exit)
)
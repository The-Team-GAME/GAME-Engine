//-----------------------------------------------------------------------------------
// file:    PlayerStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (90%)
//          Primary author
//          Chew Shee Yang (10%)
//          Initial Author
//
// email:   c.mingxian@digipen.edu
//          chew.s@digipen.edu
//
// brief:   Player state machine callback implementations (serializable)
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "pch.h"
#include "PlayerStateCallbacks.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"
#include "../Components/HealthComponents.h"
#include "../../Engine/UpgradeManager.h"
#include "../../Engine/LevelManager.h"
#include "../../Engine/SceneManager.h"

using namespace Engine;
namespace {
  f32 terminal_vel_placeholder;
  constexpr const char* LEVEL_TWO_SCENE = "LevelTwoGameplayState";
  constexpr const char* LEVEL_TWO_PATH = "levels/LevelTwo.json";
  // This bool default_states is for when we want the player still be able to give inputs (and possibly exit the state) but still keep the animation of the state
  bool default_state = false;
  bool attacking = false;
  bool on_ground_last_frame = true;

  // Pending checkpoint respawn position set when a checkpoint death triggers a full reload.
  bool s_has_pending_checkpoint_respawn = false;
  glm::vec2 s_pending_checkpoint_pos = {0.0f, 0.0f};

  // Restore transient player state without reloading the scene so defeated enemies stay dead.
  void respawn_player_at_checkpoint(ECS::Entity entity, const Math::Vec2D& respawn_point)
  {
    auto* transform = entity.get_component<Components::Transform2D>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* health = entity.get_component<Components::Health>();
    auto* player = entity.get_component<Components::Player>();

    if (transform) {
      transform->position = respawn_point;
    }

    if (rigid) {
      rigid->force = Math::Vec2D();
      rigid->acceleration = Math::Vec2D();
      rigid->velocity = Math::Vec2D();
      rigid->air_time = 0.0f;
      rigid->angular_velocity = 0.0f;
      rigid->on_ground = false;
      rigid->is_jumping = false;
      rigid->apply_force = true;
    }

    if (health) {
      health->value = health->max;
    }

    if (player) {
      player->input_lock = false;
      player->damage_immune = false;
      player->knockback_value = Math::Vec2D();
      player->dash.timer = 0.0f;
      player->dash.cd_timer = 0.0f;
      player->stunned.timer = 0.0f;
      player->uppercut.timer = 0.0f;
      player->uppercut.cd_timer = 0.0f;
      player->smash.timer = 0.0f;
      player->smash.cd_timer = 0.0f;
      player->attack1.timer = 0.0f;
      player->attack2.timer = 0.0f;
      player->attack3.timer = 0.0f;
      player->leap.timer = 0.0f;
    }

    default_state = false;
    on_ground_last_frame = true;
  }

  void log_cheat_toggle(const char* cheat_name, bool enabled)
  {
    Log::info("Player: {} {}", cheat_name, enabled ? "Enabled" : "Disabled");
  }

  void toggle_bool_cheat(bool& cheat_flag, const char* cheat_name)
  {
    cheat_flag = !cheat_flag;
    log_cheat_toggle(cheat_name, cheat_flag);
  }

  void toggle_no_cooldown_cheat(Components::Player& player_comp, const char* cheat_name)
  {
    player_comp.no_cooldown_cheat = !player_comp.no_cooldown_cheat;
    player_comp.dash.cd_timer = 0.f;
    player_comp.uppercut.cd_timer = 0.f;
    player_comp.smash.cd_timer = 0.f;
    log_cheat_toggle(cheat_name, player_comp.no_cooldown_cheat);
  }

  void jump_to_level_two_cheat()
  {
    Engine::UpgradeManager::instance().clear_level_two_entry_state();
    Engine::LevelManager::set_pending_start_level(LEVEL_TWO_PATH, true);
    Engine::SceneManager::load_scene(LEVEL_TWO_SCENE);
  }



}
namespace PlayerStateCallbacks
{
#pragma region player_inputs
  // ========================================================================
  // Taking Updates in to handle the states
  // ========================================================================
  void handle_cooldowns(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    assert(player_comp);

    // Handling cooldowns
    if (player_comp->no_cooldown_cheat)
    {
      player_comp->dash.cd_timer = 0.f;
      player_comp->uppercut.cd_timer = 0.f;
      player_comp->smash.cd_timer = 0.f;
      return;
    }

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
  }
  void take_input(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    if (!player_comp)
    {
      Log::error("PlayerStateCallbacks: Missing Player component on entity {}", entity.get_id());
      return;
    }
    if (Engine::Input::is_pressed(Engine::KeyCode::F5))
    {
      jump_to_level_two_cheat();
      return;
    }
    if (Engine::Input::is_pressed(Engine::KeyCode::F7))
    {
      toggle_no_cooldown_cheat(*player_comp, "No Cooldown");
    }
    if (Engine::Input::is_pressed(Engine::KeyCode::F8))
    {
      toggle_bool_cheat(player_comp->god_mode, "God Mode");
    }
    if (player_comp->input_lock)
    { // This is for when the player enters a menu of sorts and is not supposed to press anything that will affect their payer controls in game
      return;
    }
    if (!(take_attack_input(entity) || take_movement_input(entity)))
    {
      if (!default_state)
      {
        Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
        state_machine->change_state("Idle");
      }
    }
  }
  bool take_attack_input(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::RigidBody* player_rigid = entity.get_component<Components::RigidBody>();

    if (!state_machine || !player_rigid || !player_comp)
    {
      Log::error("PlayerStateCallbacks: Missing required components on entity {}", entity.get_id());
      return false;
    }

    if (Engine::Input::is_pressed(Engine::KeyCode::K)
      && player_comp->uppercut_ready())
    {
      state_machine->change_state("PreUppercut");
      return true;
    }
    if (Engine::Input::is_pressed(Engine::KeyCode::L)
      && player_comp->smash_ready())
    {
      if (player_rigid->on_ground || player_rigid->air_time < player_comp->coyote_jump_buffer)
      {
        state_machine->change_state("Leap");
      }
      else
      {
        state_machine->change_state("PreSmash");
      }
      return true;
    }
    if (Engine::Input::is_pressed(Engine::KeyCode::J))
    {
      state_machine->change_state("Attack1");
      return true;
    }
    return false;
  }
  bool take_movement_input(ECS::Entity entity)
  {
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::RigidBody* player_rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();

    if (!state_machine || !player_rigid || !player_comp || !flip)
    {
      Log::error("PlayerStateCallbacks: Missing required components on entity {}", entity.get_id());
      return false;
    }
    if (Input::is_held(KeyCode::ShiftLeft)
      && player_comp->dash_ready())
    {
      state_machine->change_state("Dash");
      return true;
    }
    if (Input::is_pressed(KeyCode::Space)
      && (player_rigid->on_ground || player_rigid->air_time < player_comp->coyote_jump_buffer )
      && !player_rigid->is_jumping)
    {
      state_machine->change_state("Jumping");
      return true;
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
      if (!default_state)
      {
        state_machine->change_state("Walking");
      }
      return true;
    }
    return false;
  }

  // ===========================================================================================
  // Controls the player's ability to provide inputs
  // ===========================================================================================
  void player_input_lock()
  {
    ECS::Entity player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    Components::Player* player_comp = player_entity.get_component<Components::Player>();
    if (!player_comp)
    {
      Log::error("PlayerStateCallbacks: Missing Player component on entity {}", player_entity.get_id());
      return;
    }
    player_comp->input_lock = true;
  }
  void player_input_unlock()
  {
    ECS::Entity player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    Components::Player* player_comp = player_entity.get_component<Components::Player>();
    if (!player_comp)
    {
      Log::error("PlayerStateCallbacks: Missing Player component on entity {}", player_entity.get_id());
      return;
    }
    player_comp->input_lock = false;
  }

#pragma endregion
  // ===========================================================================================
  // Takes in the parent entity as well as the attack data of the attack to generate a hitbox
  // ===========================================================================================
  ECS::Entity create_hitbox(ECS::Entity parent, Components::AttackData& atk_data)
  {
    Components::Transform2D* xform = parent.get_component<Components::Transform2D>();
    Components::Flip* flip = parent.get_component<Components::Flip>();
    assert(xform);
    assert(flip);
    ECS::Entity hitbox = Engine::ECS::create_entity()
      .add_component<Components::Attack>(static_cast<s32>(atk_data.dmg))
      .add_component<Components::AttackData>(atk_data)
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
    attack->knockback = atk_data.knockback; // we set the knockback direction on the callback function instead of here

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
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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
    rigid->on_ground = false; // by right dont need this but im scared to remove it kekw

    rigid->velocity.zero();
    rigid->force.zero();
    rigid->force.y += player_comp->jump_force;

    default_state = true;
  }

  inline void jumping_on_update(Engine::ECS::Entity entity)
  {
    handle_cooldowns(entity);
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);

    // we check for rigid->on_ground here because on the first few frames there is still a chance that the player will be counted as on gound
    if (rigid->velocity.y > 0.f)// && rigid->velocity.y >= 0.0f)
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Falling");
      return;
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
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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
    default_state = true;
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(player_comp);
    assert(rigid);
    assert(flip);

    player_comp->knockback_value = player_comp->attack1_data.knockback;
    rigid->velocity.x = flip->value.x * player_comp->attack1.speed;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->attack1_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void attack1_on_update(Engine::ECS::Entity entity) {
    handle_cooldowns(entity);

    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(state_machine);
    assert(animation);

    // Animation Cancelling
    if (animation->get_current_frame_index() >= 3) { // Allow combo after frame 4
      // Check if player pressed attack again during this combo window
      if (Engine::Input::is_held(Engine::KeyCode::J) && !animation->has_ended()) {
        // Queue up next attack
        state_machine->change_state("Attack2");
        return;
      }
    }
    if (animation->get_current_frame_index() >= 5) { // Allow combo after frame 3
      if (Input::is_held(KeyCode::D) || Input::is_held(KeyCode::A))
      {
        // Change state to idle here because it will transition to walking within 1 frame and its better to do the math there
        state_machine->change_state("Idle");
        return;
      }
    }
    // Transition to idle when attack animation completes
    if (animation->has_ended()) 
    {
      state_machine->change_state("Idle");
    }
  }

  inline void attack1_on_exit([[maybe_unused]]Engine::ECS::Entity entity) 
  {
    default_state = false;
  }
#pragma endregion

#pragma region attack 2
  // ========================================================================
  // Attack 2 State Callbacks (Second Punch)
  // ========================================================================

  inline void attack2_on_enter(Engine::ECS::Entity entity) {
    Log::info("Player: Attack 2 (Second Punch)");
    default_state = true;
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(player_comp);
    assert(rigid);
    assert(flip);

    player_comp->knockback_value = player_comp->attack2_data.knockback;
    rigid->velocity.x = flip->value.x * player_comp->attack2.speed;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->attack2_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void attack2_on_update(Engine::ECS::Entity entity) {
    handle_cooldowns(entity);
    
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(state_machine);
    assert(animation);

    // Animation Cancelling
    if (animation->get_current_frame_index() >= 3) { // Allow combo after frame 3
      // Check if player pressed attack again during this combo window
      if (Engine::Input::is_held(Engine::KeyCode::J) && !animation->has_ended()) {
        // Queue up next attack
        state_machine->change_state("PreAttack3");
        return;
      }
    }
    if (animation->get_current_frame_index() >= 5) { // Allow combo after frame 5
      if (Input::is_held(KeyCode::D) || Input::is_held(KeyCode::A))
      {// Change state to idle here because it will transition to walking within 1 frame and its better to do the math there
        state_machine->change_state("Idle");
        return;
      }
    }

    // Transition to idle when attack animation completes
    if (animation->has_ended()) 
    {
      state_machine->change_state("Idle");
    }
  }

  inline void attack2_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
    default_state = false;
  }
#pragma endregion

#pragma region attack 3
  // ========================================================================
  // Pre Attack 3 State Callbacks (Third Punch - Finisher)
  // ========================================================================

  inline void pre_attack3_on_enter(Engine::ECS::Entity entity) {
    Log::info("Player: Attack 3 (Third Punch - Finisher!)");
    default_state = true;

    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    assert(player_comp);
    assert(rigid);
    assert(flip);

    player_comp->knockback_value = player_comp->attack3_data.knockback;
    rigid->velocity.x = flip->value.x * player_comp->attack3.speed;
  }

  inline void pre_attack3_on_update(Engine::ECS::Entity entity) 
  {
    handle_cooldowns(entity);
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
      assert(sm);
      sm->change_state("Attack3");
    }
  }

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
    rigid->velocity.x = flip->value.x * player_comp->attack3.speed;
    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->attack3_data);

    Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void attack3_on_update(Engine::ECS::Entity entity) {
    handle_cooldowns(entity);
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    Components::StateMachine* sm = entity.get_component<Components::StateMachine>();
    assert(sm);
    if (animation->has_ended())
    {
      sm->change_state("Idle");
    }
    // Animation Cancelling
    if (animation->get_current_frame_index() >= 4) { // Allow combo after frame 4
      if (Input::is_held(KeyCode::D) || Input::is_held(KeyCode::A))
      {// Change state to idle here because it will transition to walking within 1 frame and its better to do the math there
        sm->change_state("Idle");
        return;
      }
    }
  }

  inline void attack3_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
    default_state = false;
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
    rigid->force.zero();
    rigid->velocity.zero();

    //=========================================================================//
    ECS::Entity hitbox = create_hitbox(entity, player_comp->uppercut_data);

    //Components::Attack* attack = hitbox.get_component<Components::Attack>();
    //assert(attack);
    //attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");
  }

  inline void pre_uppercut_on_update(ECS::Entity entity)
  {
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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
    default_state = true;
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    assert(rigid);
    rigid->apply_force = false;
    rigid->velocity = rigid->velocity * 0.05f;
  }

  inline void post_uppercut_on_update(ECS::Entity entity)
  {
    handle_cooldowns(entity);
    Components::Animation* animation = entity.get_component<Components::Animation>();
    assert(animation);
    if (animation->has_ended())
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("UppercutFalling");
    }
    take_input(entity);
  }

  inline void post_uppercut_on_exit([[maybe_unused]] ECS::Entity entity)
  {
    default_state = false;

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
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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
    handle_cooldowns(entity);
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

    Engine::Systems::add_camera_shake(0.3f); // This adds to the one in register_collision_callback("damage_enemy", "enemy") as well

    /*Components::Attack* attack = hitbox.get_component<Components::Attack>();
    assert(attack);
    attack->impact_vfx_texture = Engine::Resources::load_texture("images/VFX/VFX_ImpactBlue_Spritesheet.png");*/
  }

  inline void post_smash_on_update(ECS::Entity entity)
  {
    handle_cooldowns(entity);
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
    if (!player) return;

    const std::string& scene_name = Engine::SceneManager::get_current_scene_name();
    auto& upgrade_manager = Engine::UpgradeManager::instance();

    // LevelTwo death: restore the upgrade state captured when the player first
    // entered LevelTwo, then reload the scene so the new player gets that state.
    if (scene_name == "LevelTwoGameplayState") {
      upgrade_manager.restore_level_two_entry_state();

      const std::string& level_path = Engine::LevelManager::get_current_level_path();
      const std::string restart_level = !level_path.empty()
        ? level_path
        : std::string("levels/LevelTwo.json");
      const bool append_resource_path =
        restart_level.find(':') == std::string::npos &&
        !restart_level.starts_with("/") &&
        !restart_level.starts_with("\\");

      Log::info("Player: Restarting LevelTwo after death (level={})", restart_level);
      Engine::LevelManager::set_pending_start_level(restart_level, append_resource_path);
      Engine::SceneManager::load_scene("LevelTwoGameplayState");
      return;
    }

    // LevelOne checkpoint deaths respawn the player in-place so defeated enemies
    // stay dead and the current live upgrade state is preserved.
    if (player->has_checkpoint) {
      respawn_player_at_checkpoint(entity, player->respawn_point);
      Log::info("Player: Respawned at checkpoint ({}, {}) without reloading the level",
        player->respawn_point.x,
        player->respawn_point.y);
      return;
    }

    // No checkpoint: restart the whole level and clear the run upgrades.
    // Reload the active gameplay scene and carry the current level path through
    // the scene clear so scene-local state is reset as well.
    upgrade_manager.reset();

    const std::string& level_path = Engine::LevelManager::get_current_level_path();
    const std::string& level_id = Engine::LevelManager::get_active_level_id();

    const std::string restart_level = !level_path.empty() ? level_path : level_id;
    if (!restart_level.empty() && scene_name != "No current scene") {
      const bool append_resource_path =
        restart_level.find(':') == std::string::npos &&
        !restart_level.starts_with("/") &&
        !restart_level.starts_with("\\");

      Log::info("Player: Restarting scene after death (scene={}, level={})", scene_name, restart_level);
      Engine::LevelManager::set_pending_start_level(restart_level, append_resource_path);
      Engine::SceneManager::load_scene(scene_name);
    }

    if (!level_id.empty()) {
      Log::info("Player: Restarting level after death (level_id={})", level_id);
      Engine::LevelManager::reload_level(level_id);
    }

    else if (!level_path.empty()) {
      Log::info("Player: Restarting level after death (level_path={})", level_path);
      Engine::Resources::open_level(level_path, false);
    }

    else {
      Log::warning("Player: Could not restart level after death (no active scene/id/path)");
    }

    // Important: don't touch components after initiating a level reload.
    return;
  }
#pragma endregion

  bool PlayerStateCallbacks::consume_pending_checkpoint_respawn(glm::vec2& out_pos)
  {
    if (!s_has_pending_checkpoint_respawn) return false;
    out_pos = s_pending_checkpoint_pos;
    s_has_pending_checkpoint_respawn = false;
    return true;
  }

} // namespace PlayerStateCallbacks

// Register blueprint + all player state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(Player, "Idle",
  STATE("Idle", PlayerStateCallbacks::idle_on_enter, PlayerStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Walking", PlayerStateCallbacks::walking_on_enter, PlayerStateCallbacks::walking_on_update, PlayerStateCallbacks::walking_on_exit)
  STATE("Jumping", PlayerStateCallbacks::jumping_on_enter, PlayerStateCallbacks::jumping_on_update, PlayerStateCallbacks::jumping_on_exit)
  STATE("Falling", PlayerStateCallbacks::falling_on_enter, PlayerStateCallbacks::falling_on_update, PlayerStateCallbacks::falling_on_exit)
  STATE("Dash", PlayerStateCallbacks::dash_on_enter, PlayerStateCallbacks::dash_on_update, PlayerStateCallbacks::dash_on_exit)

  STATE("Attack1", PlayerStateCallbacks::attack1_on_enter, PlayerStateCallbacks::attack1_on_update, PlayerStateCallbacks::attack1_on_exit)
  STATE("Attack2", PlayerStateCallbacks::attack2_on_enter, PlayerStateCallbacks::attack2_on_update, PlayerStateCallbacks::attack2_on_exit)
  STATE("PreAttack3", PlayerStateCallbacks::pre_attack3_on_enter, PlayerStateCallbacks::pre_attack3_on_update, NO_CALLBACK)
  STATE("Attack3", PlayerStateCallbacks::attack3_on_enter, PlayerStateCallbacks::attack3_on_update, PlayerStateCallbacks::attack3_on_exit)

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
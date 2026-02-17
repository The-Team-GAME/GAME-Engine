#include "pch.h"
#include "States.h"
#include "../Components/GameplayComponent.h"
#include "../Components/HeartrateComponent.h"

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
  void take_input(Engine::ECS::Entity entity)
  {
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    Components::RigidBody* player_rigid = entity.get_component<Components::RigidBody>();
    Components::Player* player_comp = entity.get_component<Components::Player>();
    Components::Flip* flip = entity.get_component<Components::Flip>();
    Components::Heartrate* heartrate = entity.get_component<Components::Heartrate>();

    if (heartrate->heartrate >= heartrate->dead)
    {
      state_machine->change_state("Death");
      return;
    }

    // Handling cooldowns
    if (dash_cooldown > 0) { dash_cooldown -= Engine::Time::get_fixed_dt(); }

    // Check for attack input
    if (Engine::Input::is_pressed(Engine::KeyCode::J))
    {
      state_machine->change_state("Attack1");
      return;
    }

    // Check for movement input
    if (Engine::Input::is_held(Engine::KeyCode::ShiftLeft) && dash_cooldown <= 0.f)
    {
      state_machine->change_state("Dash");
      return;
    }

    if (Engine::Input::is_held(Engine::KeyCode::Space)
      && player_rigid->on_ground
      && !player_rigid->is_jumping)
    {
      state_machine->change_state("Jumping");
      return;
    }

    if (Engine::Input::is_held(Engine::KeyCode::D) || Engine::Input::is_held(Engine::KeyCode::A))
    {
      if (Engine::Input::is_held(Engine::KeyCode::A)) //left
      {
        flip->value.x = -1.f;
      }
      else if (Engine::Input::is_held(Engine::KeyCode::D)) //right
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

  void idle_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    /* Empty By Design */
    Log::info("Player: Entered Idle state");
  }

  void idle_on_update(Engine::ECS::Entity entity) {
    take_input(entity);
    std::cout << "Idle State Update\n";
  }
#pragma endregion

#pragma region Walking State
  // ========================================================================
  // Walking State Callbacks
  // ========================================================================

  void walking_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    Log::info("Player: Entered Walking state");

  }

  void walking_on_update(Engine::ECS::Entity entity)
  {
    // we set an exit criteria for the walking state instead of reseting it every frame as the animation sequence needs the state to be constant for this to work
    if (!Engine::Input::is_held(Engine::KeyCode::A) && !Engine::Input::is_held(Engine::KeyCode::D))
    {
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      state_machine->change_state("Idle");
    }
    take_input(entity);
  }

  void walking_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
    // Empty by design
  }
#pragma endregion

#pragma region Jumping State
  // ========================================================================
  // Jumping State Callbacks
  // ========================================================================

  void jumping_on_enter(Engine::ECS::Entity entity)
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

  void jumping_on_update(Engine::ECS::Entity entity)
  {
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    assert(rigid);
    assert(state_machine);

    if (rigid->on_ground && rigid->velocity.y >= 0.0f)
    {
      state_machine->change_state("Idle");
    }
    take_input(entity);
  }

  void jumping_on_exit(Engine::ECS::Entity entity)
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

  void dash_on_enter(Engine::ECS::Entity entity)
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

  void dash_on_update(Engine::ECS::Entity entity)
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
    player_comp->dash_timer -= Engine::Time::get_fixed_dt();
  }

  void dash_on_exit(Engine::ECS::Entity entity)
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

  void attack1_on_enter(Engine::ECS::Entity entity) {
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

  void attack1_on_update(Engine::ECS::Entity entity) {
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

  void attack2_on_enter(Engine::ECS::Entity entity) {
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

  void attack2_on_update(Engine::ECS::Entity entity) {
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

  void attack3_on_enter(Engine::ECS::Entity entity) {
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

  void attack3_on_update(Engine::ECS::Entity entity) {
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

#pragma region Death
  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  void death_on_enter([[maybe_unused]] Engine::ECS::Entity entity)
  {
    Log::info("Player: Died!");
  }

  void death_on_update(Engine::ECS::Entity entity)
  {
    Components::Animation* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      Log::info("Player: Destroyed after death animation.");
      Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
      assert(state_machine);
      state_machine->change_state("Idle");
    }
  }

  void death_on_exit([[maybe_unused]] Engine::ECS::Entity entity)
  {
    Log::info("Player: Exited Death state");
    //Components::StateMachine* state_machine = entity.get_component<Components::StateMachine>();
    //assert(state_machine);
    //state_machine->change_state("Idle");
    std::string scene = Engine::SceneManager::get_current_scene_name();
    Engine::SceneManager::load_scene(scene);
  }
#pragma endregion
} // namespace PlayerStateCallbacks

namespace PlayerStates
{
  Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Audio Setup
    // ========================================================================
    Components::StateAudio walk_audio;
    walk_audio.sound = Engine::Resources::load_sound("sounds/SFX/Player/Player Footstep.wav");
    walk_audio.loop = true;
    walk_audio.volume = 0.5f;
    walk_audio.stop_on_exit = true;

    Components::StateAudio jump_audio;
    jump_audio.sound = Engine::Resources::load_sound("sounds/SFX/Player/Player Jump (DJ SY Edition).wav");
    jump_audio.loop = false;
    jump_audio.volume = 0.5f;
    jump_audio.stop_on_exit = false;

    Components::StateAudio attack_audio;
    attack_audio.sound = Engine::Resources::load_sound("sounds/SFX/Player/Player Swing.wav");
    attack_audio.loop = false;
    attack_audio.volume = 0.2f;
    attack_audio.stop_on_exit = false;


    // ========================================================================
    // FMOD Audio Setup (NEW SYSTEM - preferred if FMOD banks are available)
    // ========================================================================
    // NOTE: Uncomment these when you have FMOD Studio events created and banks built


    Components::FMODStateAudio fmod_hurt_audio;
    fmod_hurt_audio.event_path = "event:/SFX/PlayerHurt";
    fmod_hurt_audio.loop = false;
    fmod_hurt_audio.volume = 0.6f;
    fmod_hurt_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_walking_audio;
    fmod_walking_audio.event_path = "event:/SFX/PlayerWalking";
    fmod_walking_audio.loop = false;
    fmod_walking_audio.volume = 0.6f;
    fmod_walking_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_jump_audio;
    fmod_jump_audio.event_path = "event:/SFX/PlayerJump";
    fmod_jump_audio.loop = false;
    fmod_jump_audio.volume = 0.6f;
    fmod_jump_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_dash_audio;
    fmod_dash_audio.event_path = "event:/SFX/PlayerDash";
    fmod_dash_audio.loop = false;
    fmod_dash_audio.volume = 0.6f;
    fmod_dash_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_attack_audio;
    fmod_attack_audio.event_path = "event:/SFX/PlayerPunch";
    fmod_attack_audio.loop = false;
    fmod_attack_audio.volume = 0.2f;
    fmod_attack_audio.stop_on_state_exit = false;

    // ========================================================================
    // Player Idle State
    // ========================================================================
    Components::StateAnimation player_idle_anim;
    player_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_idle_spritesheet.png"));
    player_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 24, 24);
    player_idle_anim.animation = Components::Animation(24, 0.1f);
    player_idle_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_idle("Idle", player_idle_anim);

    // ========================================================================
    // Player Walking State
    // ========================================================================
    Components::StateAnimation player_walking_anim;
    player_walking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_run_spritesheet.png"));
    player_walking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    player_walking_anim.animation = Components::Animation(8, 0.1f);
    player_walking_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_walking("Walking", player_walking_anim, walk_audio, fmod_walking_audio);

    // ========================================================================
    // Player Jumping State
    // ========================================================================
    Components::StateAnimation player_jumping_anim;
    player_jumping_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_jump_spritesheet.png"));
    player_jumping_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 13, 13);
    player_jumping_anim.animation = Components::Animation(8, 0.1f);
    player_jumping_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_jumping("Jumping", player_jumping_anim, jump_audio, fmod_jump_audio);

    // ========================================================================
    // Player Dash State
    // ========================================================================
    Components::StateAnimation player_dashinging_anim;
    player_dashinging_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_dash_spritesheet.png"));
    player_dashinging_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 9, 9);
    player_dashinging_anim.animation = Components::Animation(8, 0.1f);
    player_dashinging_anim.scale = Math::Vec2D(IDLE_WIDTH * 2.0f, IDLE_HEIGHT * 2.0f);

    Components::State player_dash("Dash", player_dashinging_anim, walk_audio, fmod_dash_audio);

    // ========================================================================
    // Player Attack 1 State (First Punch)
    // ========================================================================
    Components::StateAnimation player_attack1_anim;
    player_attack1_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_onehit_spritesheet.png"));
    player_attack1_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 9, 9);
    player_attack1_anim.animation = Components::Animation({ 0, 1, 2, 3, 4, 5, 6, 7, 8 }, { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }, false);
    player_attack1_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack1("Attack1", player_attack1_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Player Attack 2 State (Second Punch)
    // ========================================================================
    Components::StateAnimation player_attack2_anim;
    player_attack2_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_twohit_spritesheet.png"));
    player_attack2_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 13, 13);
    player_attack2_anim.animation = Components::Animation({ 6, 7, 8, 9, 10, 11, 12 }, { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }, false);
    player_attack2_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack2("Attack2", player_attack2_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Player Attack 3 State (Third Punch - Finisher)
    // ========================================================================
    Components::StateAnimation player_attack3_anim;
    player_attack3_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_threehit_spritesheet.png"));
    player_attack3_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 18, 18);
    player_attack3_anim.animation = Components::Animation({ 9, 10, 11, 12, 13, 14, 15, 16, 17 }, { 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f }, false);
    player_attack3_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    Components::State player_attack3("Attack3", player_attack3_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Player Death State
    // ========================================================================
    Components::StateAnimation player_death_anim;
    player_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/BroSpriteSheets/Bro_death_spritesheet.png"));
    player_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 12, 12);
    player_death_anim.animation = Components::Animation(8, 0.1f);
    player_death_anim.scale = Math::Vec2D(IDLE_WIDTH * 4.0f, IDLE_HEIGHT * 2.0f);

    // TODO Change Death Audio
    Components::State player_dead("Death", player_death_anim, walk_audio);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine player_sm;
    player_sm.blueprint_id = "Player";
    player_sm.add_state(player_idle);
    player_sm.add_state(player_walking);
    player_sm.add_state(player_jumping);
    player_sm.add_state(player_dash);
    player_sm.add_state(player_attack1);
    player_sm.add_state(player_attack2);
    player_sm.add_state(player_attack3);
    player_sm.add_state(player_dead);
    player_sm.set_initial_state("Idle");

    return player_sm;
  }
}

// Register blueprint + all player state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(Player, "Idle",
  STATE("Idle", PlayerStateCallbacks::idle_on_enter, PlayerStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Walking", PlayerStateCallbacks::walking_on_enter, PlayerStateCallbacks::walking_on_update, PlayerStateCallbacks::walking_on_exit)
  STATE("Jumping", PlayerStateCallbacks::jumping_on_enter, PlayerStateCallbacks::jumping_on_update, PlayerStateCallbacks::jumping_on_exit)
  STATE("Dash", PlayerStateCallbacks::dash_on_enter, PlayerStateCallbacks::dash_on_update, PlayerStateCallbacks::dash_on_exit)
  STATE("Attack1", PlayerStateCallbacks::attack1_on_enter, PlayerStateCallbacks::attack1_on_update, NO_CALLBACK)
  STATE("Attack2", PlayerStateCallbacks::attack2_on_enter, PlayerStateCallbacks::attack2_on_update, NO_CALLBACK)
  STATE("Attack3", PlayerStateCallbacks::attack3_on_enter, PlayerStateCallbacks::attack3_on_update, NO_CALLBACK)
  STATE("Death", PlayerStateCallbacks::death_on_enter, PlayerStateCallbacks::death_on_update, PlayerStateCallbacks::death_on_exit)
)

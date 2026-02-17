#include "pch.h"
#include "States.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace LazyMonkeyStateCallbacks
{
  // ========================================================================
  // Helper Function - Update knockback direction based on player position
  // ========================================================================
  void update_contact_knockback_direction(Engine::ECS::Entity entity) {
    auto* attack = entity.get_component<Components::Attack>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    auto* enemy_transform = entity.get_component<Components::Transform2D>();

    if (!attack || !lazy_monkey || !enemy_transform) return;

    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) return;

    auto* player_transform = player_entity.get_component<Components::Transform2D>();
    if (!player_transform) return;

    // Calculate direction from enemy to player
    float dir_x = player_transform->position.x - enemy_transform->position.x;
    float sign = (dir_x >= 0.0f ? 1.0f : -1.0f);

    // Set knockback with correct direction
    attack->knockback.x = sign * std::abs(lazy_monkey->knockback.x);
    attack->knockback.y = -std::abs(lazy_monkey->knockback.y); // Negative for upward
  }

  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  void idle_on_enter(Engine::ECS::Entity entity) {
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    if (lazy_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      lazy_monkey->last_attack_time = Engine::Time::get_time(); // Reusing this field as "idle_enter_time"
    }
    Log::info("Lazy Monkey: Entered Idle state");
  }

  void idle_on_update(Engine::ECS::Entity entity) {
    // Update knockback direction for contact damage
    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (!lazy_monkey) return;

    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - lazy_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!player_entity.is_valid()) {
      return;
    }

    auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* player_transform = player_entity.get_component<Components::Transform2D>();

    if (lazy_monkey_transform && player_transform) {
      float dx = player_transform->position.x - lazy_monkey_transform->position.x;
      float dy = player_transform->position.y - lazy_monkey_transform->position.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      // Start chasing if player is within detection range
      if (distance < lazy_monkey->detection_range) {
        state_machine->change_state("Chasing");
      }
    }
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================

  void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Lazy Monkey: Entered Chasing state");
  }

  void chasing_on_update(Engine::ECS::Entity entity) {
    // Update knockback direction for contact damage
    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();

    if (!lazy_monkey) return;

    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid() && rigid && lazy_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - lazy_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally, but stop when within attack range
        if (distance > lazy_monkey->attack_range) {
          float desired_vx = (dx > 0 ? lazy_monkey->chase_speed : -lazy_monkey->chase_speed);
          float dt = Engine::Time::get_fixed_dt();
          float delta_v = desired_vx - rigid->velocity.x;
          float force_x = (delta_v / dt) * rigid->mass; // F = m * dv/dt
          Engine::Systems::Physics2DSystem::add_force(*rigid, Math::Vec2D(force_x, 0.0f));
          // Update flip direction based on movement
          if (flip) {
            flip->value.x = (dx > 0 ? 1.0f : -1.0f);
          }
        }
        else {
          // Stop moving when close enough to attack
          rigid->velocity.x = 0.0f;
        }

        // Return to idle if player is far away
        if (distance > lazy_monkey->chase_range) {
          state_machine->change_state("Idle");
        }

        // If close enough and cooldown has elapsed, go to windup before attacking
        if (distance < lazy_monkey->attack_range &&
          (current_time - lazy_monkey->last_attack_time) >= lazy_monkey->attack_cooldown) {
          state_machine->change_state("Windup");
        }
      }
    }
  }

  void chasing_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {

  }

  // ========================================================================
  // Windup State Callbacks
  // ========================================================================

  void windup_on_enter(Engine::ECS::Entity entity) {
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when windup started (reuse last_attack_time)
    if (lazy_monkey) {
      lazy_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Lazy Monkey: Winding up for attack");
  }

  void windup_on_update(Engine::ECS::Entity entity) {
    // Update knockback direction for contact damage
    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!lazy_monkey) return;

    float current_time = Engine::Time::get_time();
    float windup_elapsed = current_time - lazy_monkey->last_attack_time;

    // Get player entity to face them during windup
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid()) {
      auto* lazy_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (lazy_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - lazy_monkey_transform->position.x;
        // Update flip direction to face player during wind-up
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // After windup duration, attack
    if (windup_elapsed >= lazy_monkey->attack_windup) {
      lazy_monkey->last_attack_time = current_time;
      state_machine->change_state("Attacking");
    }
  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  void attacking_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    // auto* transform = entity.get_component<Components::Transform2D>();

    if (rigid && flip && lazy_monkey && rigid->on_ground) {
      // Throw monkey forward based on facing direction
      rigid->velocity.x = (flip->value.x > 0 ? lazy_monkey->leap_speed : -lazy_monkey->leap_speed);
      // Add upward velocity for a leap effect
      rigid->velocity.y = -lazy_monkey->leap_height; // Negative is up
    }

    // Knockback direction is now handled by update_contact_knockback_direction in update

    Log::info("Lazy Monkey: Entered Attacking state");
  }


  void attacking_on_update(Engine::ECS::Entity entity) {
    // Update knockback direction for contact damage
    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // Transition back to chasing when attack animation completes
    if (animation && animation->has_ended()) {
      state_machine->change_state("Chasing");
    }
  }

  void attacking_on_exit(Engine::ECS::Entity entity) {
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    if (lazy_monkey) {
      // Update last attack time for cooldown
      lazy_monkey->last_attack_time = Engine::Time::get_time();
    }
  }

  //// ========================================================================
  //// Hit State Callbacks
  //// ========================================================================

  //static inline void emit_hit_particles(const Engine::ECS::Entity enemy) {
  //  using namespace Engine;
  //  using Components::Transform2D;

  //  const auto player = Engine::ECS::get_first_entity_with<Components::Player>();
  //  const auto* et = enemy.get_component<Transform2D>();
  //  if (!et) return;

  //  Systems::SetParticleParamsEvent set{};
  //  set.spread_deg = 55.f;     // cone
  //  set.rate = 0.f;      // burst only
  //  set.playing = false;

  //  Math::Vec2D spawn = et->position;

  //  // Aim away from attacker if player is valid
  //  if (player.is_valid()) {
  //    if (const auto* pt = player.get_component<Transform2D>()) {
  //      // Vector from attacker -> enemy
  //      const float vx = et->position.x - pt->position.x;
  //      const float vy = et->position.y - pt->position.y;

  //      // Normalized hit-normal (away from attacker)
  //      const float len = std::max(1e-4f, std::sqrt(vx * vx + vy * vy));
  //      const float nx = vx / len;
  //      const float ny = vy / len;

  //      // Emission angle: along the normal (screen up is -Y)
  //      const float deg = std::atan2(-ny, nx) * 57.29577951308232f;
  //      set.angle_deg = deg;

  //      const float lateral_offset_px = 15.f;  // adjust offset horizontal
  //      const float vertical_offset_px = 0.f;  // adjust offset vertical
  //      spawn.x += nx * lateral_offset_px;
  //      spawn.y += ny * lateral_offset_px + vertical_offset_px;
  //    }
  //  }

  //  // Post to the particle system
  //  Engine::Systems::ParticleSystem::post(set);

  //  Systems::SpawnParticlesEvent burst{};
  //  burst.pos = spawn;
  //  burst.count = 100;
  //  Engine::Systems::ParticleSystem::post(burst);
  //}

  void hit_on_enter(Engine::ECS::Entity entity) {
    (void)entity; // Suppress unused parameter warning



    Log::info("Lazy Monkey: Stunned!");
  }

  void hit_on_update(Engine::ECS::Entity entity) {
    // Update knockback direction for contact damage
    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* lazy_monkey = entity.get_component<Components::LazyMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!lazy_monkey) return;

    // When animation ends, detach from hitbox and apply knockback
    if (animation->has_ended() && pending_knockback && rigid) {
      // Detach from hitbox first (stop following player)
      if (entity.has_component<Components::Parent>()) {
        entity.remove_component<Components::Parent>();
      }

      // Apply knockback using physics system
      Engine::Systems::Physics2DSystem::add_force(*rigid, pending_knockback->knockback);

      // Remove the pending knockback component (one-time use)
      entity.remove_component<Components::PendingKnockback>();
    }

    // Transition to idle only when grounded (after knockback has been applied)
    if (animation->has_ended() && rigid && rigid->on_ground) {
      state_machine->change_state("Idle");
    }
  }

  void hit_on_exit(Engine::ECS::Entity entity) {
    // Cleanup: ensure parent is removed (safety check)
    if (entity.has_component<Components::Parent>()) {
      entity.remove_component<Components::Parent>();
    }
  }

  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  void death_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Lazy Monkey: Died!");
  }

  void death_on_update(Engine::ECS::Entity entity) {
    auto* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      Log::info("Lazy Monkey: Destroyed after death animation.");
    }
  }

  void death_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    // Should not happen unless respawning
  }

} // namespace LazyMonkeyStateCallbacks

namespace LazyMonkeyStates
{
  Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Audio Setup (OLD SYSTEM - kept as fallback)
    // ========================================================================
    Components::StateAudio footstep_audio;
    footstep_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey SFX/Monkey FootSteps.wav");
    footstep_audio.loop = true;
    footstep_audio.volume = 0.5f;
    footstep_audio.stop_on_exit = true;

    Components::StateAudio attack_audio;
    attack_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-2_2_Attack.wav");
    attack_audio.loop = false;
    attack_audio.volume = 0.6f;
    attack_audio.stop_on_exit = false;

    Components::StateAudio hurt_audio;
    /* hurt_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-1_8_Hurt.wav");
     hurt_audio.loop = false;
     hurt_audio.volume = 0.6f;
     hurt_audio.stop_on_exit = false;*/



     // ========================================================================
     // FMOD Audio Setup (NEW SYSTEM - preferred if FMOD banks are available)
     // ========================================================================
     // NOTE: Uncomment these when you have FMOD Studio events created and banks built

    Components::FMODStateAudio fmod_footstep_audio;
    fmod_footstep_audio.event_path = "event:/SFX/MonkeyFootsteps";
    fmod_footstep_audio.loop = true;
    fmod_footstep_audio.volume = 0.5f;
    fmod_footstep_audio.stop_on_state_exit = true;

    Components::FMODStateAudio fmod_attack_audio;
    fmod_attack_audio.event_path = "event:/SFX/MonkeyAttack";
    fmod_attack_audio.loop = false;
    fmod_attack_audio.volume = 0.6f;
    fmod_attack_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_hurt_audio;
    fmod_hurt_audio.event_path = "event:/SFX/MonkeyHurt";
    fmod_hurt_audio.loop = false;
    fmod_hurt_audio.volume = 0.6f;
    fmod_hurt_audio.stop_on_state_exit = false;

    Components::FMODStateAudio fmod_chase_audio;
    fmod_chase_audio.event_path = "event:/SFX/MonkeyChasing";
    fmod_chase_audio.loop = false;
    fmod_chase_audio.volume = 0.6f;
    fmod_chase_audio.stop_on_state_exit = false;

    // ========================================================================
    // Lazy Monkey Idle State
    // ========================================================================
    Components::StateAnimation lazy_monkey_idle_anim;
    lazy_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"));
    lazy_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    lazy_monkey_idle_anim.animation = Components::Animation(5, 0.15f);
    lazy_monkey_idle_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_idle("Idle", lazy_monkey_idle_anim);

    // ========================================================================
    // Lazy Monkey Chasing State
    // ========================================================================
    Components::StateAnimation lazy_monkey_chasing_anim;
    lazy_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Walk_Spritesheet.png"));
    lazy_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    lazy_monkey_chasing_anim.animation = Components::Animation(6, 0.1f);
    lazy_monkey_chasing_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    // Try FMOD first, fallback to old audio if FMOD fails
    Components::State lazy_monkey_chasing("Chasing", lazy_monkey_chasing_anim, footstep_audio, fmod_chase_audio);

    // ========================================================================
    // Lazy Monkey Windup State (uses idle animation)
    // ========================================================================
    Components::StateAnimation lazy_monkey_windup_anim;
    lazy_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Idle_Spritesheet.png"));
    lazy_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 5, 5);
    lazy_monkey_windup_anim.animation = Components::Animation(5, 0.15f);
    lazy_monkey_windup_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_windup("Windup", lazy_monkey_windup_anim);

    // ========================================================================
    // Lazy Monkey Attacking State
    // ========================================================================
    Components::StateAnimation lazy_monkey_attacking_anim;
    lazy_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Attack_Spritesheet.png"));
    lazy_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(96, 64, 1, 5, 5);
    lazy_monkey_attacking_anim.animation = Components::Animation({ 0.2f, 0.1f, 0.1f, 0.1f, 0.5f });
    lazy_monkey_attacking_anim.scale = Math::Vec2D(ATTACK_WIDTH * 2.0f, ATTACK_HEIGHT * 2.0f);

    // Try FMOD first, fallback to old audio if FMOD fails
    Components::State lazy_monkey_attacking("Attacking", lazy_monkey_attacking_anim, attack_audio, fmod_attack_audio);

    // ========================================================================
    // Lazy Monkey Hit State
    // ========================================================================
    Components::StateAnimation lazy_monkey_hit_anim;
    lazy_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Damaged_Spritesheet.png"));
    lazy_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 3, 3);
    lazy_monkey_hit_anim.animation = Components::Animation(std::initializer_list{ 2 }, { 0.4f });
    lazy_monkey_hit_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    // FMOD only (no fallback) - old audio commented out above
    Components::State lazy_monkey_hit("Hit", lazy_monkey_hit_anim, std::nullopt, fmod_hurt_audio);

    // ========================================================================
    // Lazy Monkey Death State
    // ========================================================================
    Components::StateAnimation lazy_monkey_death_anim;
    lazy_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/LazyMonkeySpriteSheets/LazyMonkey_Death_Spritesheet.png"));
    lazy_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 8, 8);
    // lazy_monkey_death_anim.animation = Components::Animation({ 0.2f, 0.1f, 1.5f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f });
    lazy_monkey_death_anim.animation = Components::Animation({ 0.2f, 0.1f, 1.5f });
    lazy_monkey_death_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State lazy_monkey_death("Death", lazy_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine lazy_monkey_sm;
    lazy_monkey_sm.blueprint_id = "LazyMonkey";
    lazy_monkey_sm.add_state(lazy_monkey_idle);
    lazy_monkey_sm.add_state(lazy_monkey_chasing);
    lazy_monkey_sm.add_state(lazy_monkey_windup);
    lazy_monkey_sm.add_state(lazy_monkey_attacking);
    lazy_monkey_sm.add_state(lazy_monkey_hit);
    lazy_monkey_sm.add_state(lazy_monkey_death);
    lazy_monkey_sm.set_initial_state("Idle");

    return lazy_monkey_sm;
  }
}

// Register blueprint + all lazy monkey state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(LazyMonkey, "Idle",
  STATE("Idle", LazyMonkeyStateCallbacks::idle_on_enter, LazyMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("Chasing", LazyMonkeyStateCallbacks::chasing_on_enter, LazyMonkeyStateCallbacks::chasing_on_update, LazyMonkeyStateCallbacks::chasing_on_exit)
  STATE("Windup", LazyMonkeyStateCallbacks::windup_on_enter, LazyMonkeyStateCallbacks::windup_on_update, NO_CALLBACK)
  STATE("Attacking", LazyMonkeyStateCallbacks::attacking_on_enter, LazyMonkeyStateCallbacks::attacking_on_update, LazyMonkeyStateCallbacks::attacking_on_exit)
  STATE("Hit", LazyMonkeyStateCallbacks::hit_on_enter, LazyMonkeyStateCallbacks::hit_on_update, LazyMonkeyStateCallbacks::hit_on_exit)
  STATE("Death", LazyMonkeyStateCallbacks::death_on_enter, LazyMonkeyStateCallbacks::death_on_update, LazyMonkeyStateCallbacks::death_on_exit)
)

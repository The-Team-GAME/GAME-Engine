#include "pch.h"
#include "States.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace SprayCanMonkey
{
  // Helper function to update knockback direction based on player position
  void update_contact_knockback_direction(Engine::ECS::Entity entity) {
    auto* attack = entity.get_component<Components::Attack>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* enemy_transform = entity.get_component<Components::Transform2D>();

    if (!attack || !spray_monkey || !enemy_transform) return;

    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) return;

    auto* player_transform = player_entity.get_component<Components::Transform2D>();
    if (!player_transform) return;

    // Calculate direction from enemy to player
    float dir_x = player_transform->position.x - enemy_transform->position.x;
    float sign = (dir_x >= 0.0f ? 1.0f : -1.0f);

    // Set knockback with correct direction
    attack->knockback.x = sign * std::abs(spray_monkey->knockback.x);
    attack->knockback.y = -std::abs(spray_monkey->knockback.y); // Negative for upward
  }

  Engine::ECS::Entity spawn_spray_cloud(Engine::ECS::Entity monkey)
  {
    auto* spray = monkey.get_component<Components::SprayCanMonkey>();
    auto* transform = monkey.get_component<Components::Transform2D>();
    auto* flip = monkey.get_component<Components::Flip>();

    if (!spray || !transform) return {};

    float dir = (flip && flip->value.x < 0.f) ? -1.f : 1.f;

    Math::Vec2D cloud_pos = transform->position;
    cloud_pos.x += dir * (spray->spray_range * 1.0f);

    //   // Use hardcoded values instead of component fields
    //   const float w = 160.0f;  // was: spray->cloud_width
    //   const float h = 160.0f;   // was: spray->cloud_height

    //   const float cloud_life = 4.0f;
    //   const float start_vx = dir * 150.0f;

    //   Engine::ECS::Entity cloud = Engine::ECS::create_entity()
    //     .add_component<Components::Tag<"SprayCloud">>()
    //     .add_component<Components::Attack>(
    //       1,  // was: spray->cloud_damage_per_tick
    //       Math::Vec2D(10.f, 10.f),
    //       true,                        // is_continuous
    //       0.5f  // was: spray->cloud_damage_interval
    //     )
    //     .add_component<Components::Transform2D>(cloud_pos.x, cloud_pos.y, 0.0f, w, h)
    //     .add_component<Components::RigidBody>(true, false)
    //     .add_component<Components::BoxCollider>(w, h, false)
    //     .add_component<Components::Lifetime>(4.0f)  // was: spray->cloud_duration
    //     .add_component<Components::Texture>(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/GreenCloud.png"))
    //     .add_component<Components::Color>(Math::Vec4D(1.f, 1.f, 1.f, 1.f))
    //     .remove_from_layer("default")
    //     .add_to_layer("damage_player")
    //     .add_to_layer("spray");

    const float start_vx = dir * 150.0f;

    // Spawn spray cloud from prefab
    auto prefab = Engine::Resources::load_prefab("prefabs/spray_cloud.json");
    Engine::ECS::Entity cloud = Engine::ECS::instantiate_prefab(prefab);

    // Add lifetime component (despawn after 4 seconds)
    cloud.add_component<Components::Lifetime>(4.0f);

    // Set spawn position
    if (auto* cloud_transform = cloud.get_component<Components::Transform2D>()) {
      cloud_transform->position = cloud_pos;
    }

    // Set velocity
    if (auto* rb = cloud.get_component<Components::RigidBody>()) {
      rb->velocity.x = start_vx;
    }

    return cloud;
  }

  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  void idle_on_enter(Engine::ECS::Entity entity) {
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    if (spray_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      spray_monkey->last_attack_time = Engine::Time::get_time();
    }
    Log::info("Spray Can Monkey: Entered Idle state");
  }

  void idle_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();

    if (!spray_monkey) return;

    update_contact_knockback_direction(entity);
    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - spray_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!player_entity.is_valid()) {
      return;
    }

    auto* monkey_transform = entity.get_component<Components::Transform2D>();
    auto* player_transform = player_entity.get_component<Components::Transform2D>();

    if (monkey_transform && player_transform) {
      float dx = player_transform->position.x - monkey_transform->position.x;
      float dy = player_transform->position.y - monkey_transform->position.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      if (distance < spray_monkey->detection_range) {
        state_machine->change_state("Chasing");
      }
    }
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================

  void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Spray Can Monkey: Entered Chasing state");
  }

  void chasing_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* spray_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();

    if (!spray_monkey) return;


    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    update_contact_knockback_direction(entity);

    if (player_entity.is_valid() && rigid && spray_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - spray_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally, but stop when within attack range
        if (distance > spray_monkey->attack_range) {
          rigid->velocity.x = (dx > 0 ? spray_monkey->chase_speed : -spray_monkey->chase_speed);
          // Update flip direction based on movement
          if (flip) {
            flip->value.x = (dx > 0 ? 1.0f : -1.0f);
          }
          // Don't modify velocity.y - let gravity handle it
        }
        else {
          // Stop moving when close enough to attack
          rigid->velocity.x = 0.0f;
        }

        // Return to idle if player is far away
        if (distance > spray_monkey->chase_range) {
          state_machine->change_state("Idle");
        }

        // If close enough and cooldown has elapsed, go to windup before attacking
        if (distance < spray_monkey->attack_range &&
          (current_time - spray_monkey->last_attack_time) >= spray_monkey->attack_cooldown) {
          state_machine->change_state("Windup");
        }
      }
    }
  }

  void chasing_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) rigid->velocity.x = 0.0f;
  }

  // ========================================================================
  // Windup State Callbacks
  // ========================================================================

  void windup_on_enter(Engine::ECS::Entity entity) {
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when windup started
    if (spray_monkey) {
      spray_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Spray Can Monkey: Winding up for attack");
  }

  void windup_on_update(Engine::ECS::Entity entity) {

    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!spray_monkey) return;

    float current_time = Engine::Time::get_time();
    float windup_elapsed = current_time - spray_monkey->last_attack_time;

    // Get player entity to face them during windup
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid()) {
      auto* spray_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (spray_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - spray_monkey_transform->position.x;
        // Update flip direction to face player during wind-up
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // After windup duration, attack
    if (windup_elapsed >= spray_monkey->attack_windup) {
      spray_monkey->last_attack_time = current_time;
      state_machine->change_state("Spraying");
    }
  }

  // ========================================================================
  // Spraying State Callbacks
  // ========================================================================

  void spraying_on_enter(Engine::ECS::Entity entity) {
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();

    if (spray_monkey) {
      spray_monkey->last_attack_time = Engine::Time::get_time();
      spray_monkey->cloud_spawned = false;
      //spawn_spray_cloud(entity);
    }

    Log::info("Spray Can Monkey: Spraying!");
  }

  void spraying_on_update(Engine::ECS::Entity entity) {

    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* animation = entity.get_component<Components::Animation>();

    if (!spray_monkey) return;

    float current_time = Engine::Time::get_time();
    float spray_elapsed = current_time - spray_monkey->last_attack_time;

    if (!spray_monkey->cloud_spawned && spray_elapsed >= spray_monkey->cloud_spawn_delay) {
      auto cloud = spawn_spray_cloud(entity);

      spray_monkey->cloud_entity = cloud;
      spray_monkey->cloud_spawn_time = current_time;

      spray_monkey->cloud_initial_vx =
        (cloud.get_component<Components::RigidBody>())
        ? cloud.get_component<Components::RigidBody>()->velocity.x
        : 0.f;

      spray_monkey->cloud_spawned = true;
    }

    if (spray_monkey->cloud_spawned && spray_monkey->cloud_entity.is_valid())
    {
      float tempst = (spray_elapsed) / spray_monkey->spray_duration;

      // Slow down
      if (auto* rb = spray_monkey->cloud_entity.get_component<Components::RigidBody>())
      {
        rb->velocity.x = spray_monkey->cloud_initial_vx * (0.8f - tempst);
      }

      if (!(spray_elapsed >= (spray_monkey->cloud_spawn_delay - 2.0f))) {
        if (tempst >= 1.0f) tempst = 1.0f;
        // Fade out
        if (auto* color = spray_monkey->cloud_entity.get_component<Components::Color>())
        {
          color->w -= tempst / 10.0f;   // alpha fades to 0
        }
      }

    }

    // End spray after duration or animation completes
    if (spray_elapsed >= spray_monkey->spray_duration || (animation && animation->has_ended())) {
      state_machine->change_state("Chasing");
    }
  }

  void spraying_on_exit(Engine::ECS::Entity entity) {
    // Note: Cloud entity manages its own lifetime
    // No need to manually clean up here
    if (auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>())
    {
      spray_monkey->cloud_entity = {};
      spray_monkey->cloud_spawned = false;
    }
  }

  // ========================================================================
  // Hit State Callbacks
  // ========================================================================

  void hit_on_enter(Engine::ECS::Entity) {
    Log::info("Spray Can Monkey: Hit!");
  }

  void hit_on_update(Engine::ECS::Entity entity) {

    update_contact_knockback_direction(entity);

    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* spray_monkey = entity.get_component<Components::SprayCanMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!spray_monkey) return;

    // When animation ends, detach from hitbox and apply knockback
    if (animation && animation->has_ended() && pending_knockback && rigid) {
      // Detach from hitbox first (stop following player)
      if (entity.has_component<Components::Parent>()) {
        entity.remove_component<Components::Parent>();
      }

      // Apply knockback momentum from Player struct
      rigid->velocity = pending_knockback->knockback;

      // Remove the pending knockback component (one-time use)
      entity.remove_component<Components::PendingKnockback>();
    }

    // Transition to idle only when grounded (after knockback has been applied)
    if (animation && animation->has_ended() && rigid && rigid->on_ground) {
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
    Log::info("Spray Can Monkey: Died!");
  }

  void death_on_update(Engine::ECS::Entity entity) {
    auto* animation = entity.get_component<Components::Animation>();
    auto* health = entity.get_component<Components::EnemyHealth>();

    if (health && health->current_hp <= 0) {
      if (animation && animation->has_ended()) {

        Engine::ECS::destroy_entity(entity);
        Log::info("Spray Can Monkey: Destroyed after death animation.");
      }
    }
  }

  void death_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    // Should not happen unless respawning
  }

} // namespace SprayCanMonkey

namespace SprayCanMonkeyStates
{
  Components::StateMachine create_state_machine()
  {
    // ========================================================================
    // Audio Setup
    // ========================================================================
    Components::StateAudio footstep_audio;
    footstep_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey SFX/Monkey FootSteps.wav");
    footstep_audio.loop = true;
    footstep_audio.volume = 0.5f;
    footstep_audio.stop_on_exit = true;

    Components::StateAudio spray_audio;
    spray_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-2_2_Attack.wav");
    spray_audio.loop = false;
    spray_audio.volume = 0.6f;
    spray_audio.stop_on_exit = false;

    Components::StateAudio hurt_audio;
    hurt_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-1_8_Hurt.wav");
    hurt_audio.loop = false;
    hurt_audio.volume = 0.6f;
    hurt_audio.stop_on_exit = false;

    // ========================================================================
    // Spray Can Monkey Idle State
    // ========================================================================
    Components::StateAnimation spray_monkey_idle_anim;
    spray_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_idle_anim.animation = Components::Animation(5, 0.15f);
    spray_monkey_idle_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_idle("Idle", spray_monkey_idle_anim);

    // ========================================================================
    // Spray Can Monkey Chasing State
    // ========================================================================
    Components::StateAnimation spray_monkey_chasing_anim;
    spray_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_chasing_anim.animation = Components::Animation(6, 0.1f);
    spray_monkey_chasing_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_chasing("Chasing", spray_monkey_chasing_anim, footstep_audio);

    // ========================================================================
    // Spray Can Monkey Windup State
    // ========================================================================
    Components::StateAnimation spray_monkey_windup_anim;
    spray_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Walk_Spritesheet.png"));
    spray_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    spray_monkey_windup_anim.animation = Components::Animation(4, 0.1f);
    spray_monkey_windup_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_windup("Windup", spray_monkey_windup_anim);

    // ========================================================================
    // Spray Can Monkey Spraying State
    // ========================================================================
    Components::StateAnimation spray_monkey_spraying_anim;
    spray_monkey_spraying_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Attack_Spritesheet.png"));
    spray_monkey_spraying_anim.texture_atlas = Components::TextureAtlas(80, 64, 1, 15, 15);
    spray_monkey_spraying_anim.animation = Components::Animation({ 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f });
    spray_monkey_spraying_anim.scale = Math::Vec2D(WIDTH * 3.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_spraying("Spraying", spray_monkey_spraying_anim, spray_audio);

    // ========================================================================
    // Spray Can Monkey Hit State
    // ========================================================================
    Components::StateAnimation spray_monkey_hit_anim;
    spray_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Damaged_Spritesheet.png"));
    spray_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 1, 1);
    spray_monkey_hit_anim.animation = Components::Animation(1, 0.3f);
    spray_monkey_hit_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_hit("Hit", spray_monkey_hit_anim, hurt_audio);

    // ========================================================================
    // Spray Can Monkey Death State
    // ========================================================================
    Components::StateAnimation spray_monkey_death_anim;
    spray_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/SprayPaintMonkeySpriteSheets/Spraypaint_Damaged_Spritesheet.png"));
    spray_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 1, 1);
    spray_monkey_death_anim.animation = Components::Animation(1, 0.5f);
    spray_monkey_death_anim.scale = Math::Vec2D(WIDTH * 2.0f, HEIGHT * 2.0f);

    Components::State spray_monkey_death("Death", spray_monkey_death_anim);

    // ========================================================================
    // Build and return state machine (without reloading state)
    // ========================================================================
    Components::StateMachine spray_monkey_sm;
    spray_monkey_sm.blueprint_id = "SprayCanMonkey";
    spray_monkey_sm.add_state(spray_monkey_idle);
    spray_monkey_sm.add_state(spray_monkey_chasing);
    spray_monkey_sm.add_state(spray_monkey_windup);
    spray_monkey_sm.add_state(spray_monkey_spraying);
    spray_monkey_sm.add_state(spray_monkey_hit);
    spray_monkey_sm.add_state(spray_monkey_death);
    spray_monkey_sm.set_initial_state("Idle");

    return spray_monkey_sm;
  }
}

// Register blueprint + all spray can monkey state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(SprayCanMonkey, "Idle",
  STATE("Idle", SprayCanMonkey::idle_on_enter, SprayCanMonkey::idle_on_update, NO_CALLBACK)
  STATE("Chasing", SprayCanMonkey::chasing_on_enter, SprayCanMonkey::chasing_on_update, SprayCanMonkey::chasing_on_exit)
  STATE("Windup", SprayCanMonkey::windup_on_enter, SprayCanMonkey::windup_on_update, NO_CALLBACK)
  STATE("Spraying", SprayCanMonkey::spraying_on_enter, SprayCanMonkey::spraying_on_update, SprayCanMonkey::spraying_on_exit)
  STATE("Hit", SprayCanMonkey::hit_on_enter, SprayCanMonkey::hit_on_update, SprayCanMonkey::hit_on_exit)
  STATE("Death", SprayCanMonkey::death_on_enter, SprayCanMonkey::death_on_update, SprayCanMonkey::death_on_exit)
)

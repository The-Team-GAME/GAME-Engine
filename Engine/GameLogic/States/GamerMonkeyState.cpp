#include "pch.h"
#include "States.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace GamerMonkeyStateCallbacks
{
  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  void idle_on_enter(Engine::ECS::Entity entity) {
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    if (Gamer_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      Gamer_monkey->last_attack_time = Engine::Time::get_time();
    }

    // Stop movement when entering idle
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    Log::info("Gamer Monkey: Entered Idle state");
  }

  void idle_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    if (!Gamer_monkey || !state_machine) return;

    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - Gamer_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!player_entity.is_valid()) {
      return;
    }

    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* player_transform = player_entity.get_component<Components::Transform2D>();

    if (Gamer_monkey_transform && player_transform) {
      float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
      float distance = std::abs(dx); // Use horizontal distance for sidescroller

      // Check if dash is ready
      bool dash_ready = (current_time - Gamer_monkey->last_dash_time) >= Gamer_monkey->dash_cooldown;

      // If player is within chase range, decide whether to dash or chase
      if (distance < Gamer_monkey->chase_range) {
        // If player is far (beyond close attack range) and dash is ready, do dash windup
        if (distance > Gamer_monkey->close_attack_range && dash_ready) {
          state_machine->change_state("DashWindup");
        }
        // Otherwise just chase normally
        else if (distance > Gamer_monkey->close_attack_range) {
          state_machine->change_state("Chasing");
        }
        // If very close, go straight to chasing (will attack)
        else {
          state_machine->change_state("Chasing");
        }
      }
    }
  }

  // ========================================================================
  // Dash Windup State Callbacks
  // ========================================================================
  void dash_windup_on_enter(Engine::ECS::Entity entity) {
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    if (!Gamer_monkey || !Gamer_monkey_transform) return;

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when dash windup started
    Gamer_monkey->dash_windup_start_time = Engine::Time::get_time();

    // Get player position to determine direction
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    float player_x = Gamer_monkey_transform->position.x; // Default to monkey position

    if (player_entity.is_valid()) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();
      if (player_transform) {
        player_x = player_transform->position.x;

        // Face the player
        if (flip) {
          float dx = player_x - Gamer_monkey_transform->position.x;
          flip->value.x = (dx >= 0.0f ? 1.0f : -1.0f);
        }
      }
    }

    // Create the warning indicator rectangle entity
    float monkey_x = Gamer_monkey_transform->position.x;
    float monkey_y = Gamer_monkey_transform->position.y;
    float direction = (player_x >= monkey_x) ? 1.0f : -1.0f;

    // Start with a small width rectangle at the monkey's position
    float initial_width = 50.0f;
    float indicator_x = monkey_x + (direction * initial_width * 0.5f);

    // Create indicator entity with a colored rectangle (uses default white texture with color tint)
    Gamer_monkey->dash_indicator_entity = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(
        indicator_x,
        monkey_y,
        0.0f,
        initial_width,
        Gamer_monkey->dash_indicator_height
      )
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/white.png")) // Use a white texture for colored rect
      .add_component<Components::Color>(Math::Vec4D(1.0f, 0.3f, 0.3f, 0.5f)); // Semi-transparent red

    // Store initial direction for the indicator
    Gamer_monkey->dash_target_x = player_x; // Temporarily store player x for direction

    Log::info("Gamer Monkey: Entered Dash Windup state");
  }

  void dash_windup_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!Gamer_monkey || !Gamer_monkey_transform || !state_machine) return;

    float current_time = Engine::Time::get_time();
    float elapsed = current_time - Gamer_monkey->dash_windup_start_time;
    float progress = std::min(elapsed / Gamer_monkey->dash_windup_duration, 1.0f);

    // Get current player position
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) {
      state_machine->change_state("Idle");
      return;
    }

    auto* player_transform = player_entity.get_component<Components::Transform2D>();
    if (!player_transform) {
      state_machine->change_state("Idle");
      return;
    }

    float monkey_x = Gamer_monkey_transform->position.x;
    float monkey_y = Gamer_monkey_transform->position.y;
    float player_x = player_transform->position.x;
    float direction = (player_x >= monkey_x) ? 1.0f : -1.0f;

    // Update flip to face player
    if (flip) {
      flip->value.x = direction;
    }

    // Calculate target distance and current rectangle width based on progress
    float distance_to_player = std::abs(player_x - monkey_x);
    float min_width = 50.0f;
    float target_width = distance_to_player + 50.0f; // Extend slightly past player
    float current_width = min_width + (target_width - min_width) * progress;

    // Update the warning indicator rectangle
    if (Gamer_monkey->dash_indicator_entity.is_valid()) {
      auto* indicator_transform = Gamer_monkey->dash_indicator_entity.get_component<Components::Transform2D>();
      if (indicator_transform) {
        // Position the rectangle so it starts at monkey and extends toward player
        float indicator_x = monkey_x + (direction * current_width * 0.5f);
        indicator_transform->position.x = indicator_x;
        indicator_transform->position.y = monkey_y;
        indicator_transform->scale.x = current_width;
        indicator_transform->scale.y = Gamer_monkey->dash_indicator_height;
      }

      // Make the indicator flash/pulse by adjusting alpha and color intensity
      auto* indicator_color = Gamer_monkey->dash_indicator_entity.get_component<Components::Color>();
      if (indicator_color) {
        // Pulse faster as progress increases, getting more intense
        float pulse_speed = 10.0f + progress * 20.0f;
        float alpha = 0.4f + 0.3f * std::sin(current_time * pulse_speed);
        float red_intensity = 0.8f + 0.2f * progress; // Gets more red as it progresses
        indicator_color->r = red_intensity;
        indicator_color->g = 0.2f * (1.0f - progress);
        indicator_color->b = 0.2f * (1.0f - progress);
        indicator_color->a = alpha;
      }
    }

    // When windup is complete, store the target position and transition to dash
    if (progress >= 1.0f) {
      // Store the player's current X as the dash target (where the monkey will dash to)
      Gamer_monkey->dash_target_x = player_x;
      state_machine->change_state("Dash");
    }
  }

  void dash_windup_on_exit(Engine::ECS::Entity entity) {
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    // Destroy the warning indicator entity
    if (Gamer_monkey && Gamer_monkey->dash_indicator_entity.is_valid()) {
      Engine::ECS::destroy_entity(Gamer_monkey->dash_indicator_entity);
      Gamer_monkey->dash_indicator_entity = Engine::ECS::Entity(); // Reset to invalid
    }

    Log::info("Gamer Monkey: Exited Dash Windup state");
  }

  // ========================================================================
  // Dashing State Callbacks
  // ========================================================================
  void dashing_on_enter(Engine::ECS::Entity entity) {
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    if (!Gamer_monkey) return;

    // Record when dash started (for cooldown tracking)
    Gamer_monkey->last_dash_time = Engine::Time::get_time();

    // Calculate direction to dash target
    float dx = 0.0f;
    if (Gamer_monkey_transform) {
      dx = Gamer_monkey->dash_target_x - Gamer_monkey_transform->position.x;
    }

    // Face toward the stored dash target position
    if (flip) {
      flip->value.x = (dx >= 0.0f ? 1.0f : -1.0f);
    }

    // Apply dash velocity toward the stored target X position
    if (rigid) {
      // Use a high dash speed to ensure reaching the target
      rigid->velocity.x = (dx >= 0.0f ? Gamer_monkey->dash_speed : -Gamer_monkey->dash_speed);
      // Small upward boost for visual effect
      rigid->velocity.y = -100.0f;
    }

    // Make the monkey itself deal damage on collision while dashing
    if (!entity.has_component<Components::Attack>()) {
      entity.add_component<Components::Attack>(Gamer_monkey->damage);
      auto* attack = entity.get_component<Components::Attack>();
      if (attack && flip) {
        attack->knockback = Gamer_monkey->knockback;
        if (flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        attack->knockback.y = -std::abs(attack->knockback.y);
      }
    }
    entity.add_to_layer("damage_player");

    // Note: Logging target X here previously used std::to_string in a header, which caused a compile-time
    // constant expression issue on some compilers. If needed, log from a .cpp or use a simpler message.
    Log::info("Gamer Monkey: Entered Dash state");
  }

  void dashing_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    if (!Gamer_monkey || !state_machine) return;

    if (!rigid || !Gamer_monkey_transform) {
      state_machine->change_state("Idle");
      return;
    }

    // Keep applying dash velocity (in case friction slows it down)
    float current_x = Gamer_monkey_transform->position.x;
    float target_x = Gamer_monkey->dash_target_x;
    float dx = target_x - current_x;

    // Maintain dash speed
    if (std::abs(dx) > 10.0f) {
      rigid->velocity.x = (dx >= 0.0f ? Gamer_monkey->dash_speed : -Gamer_monkey->dash_speed);
    }

    float velocity_dir = (rigid->velocity.x >= 0.0f) ? 1.0f : -1.0f;

    // If moving right and passed target, or moving left and passed target
    bool reached_target = (velocity_dir > 0.0f && current_x >= target_x) ||
      (velocity_dir < 0.0f && current_x <= target_x);

    // Also check if we're very close to target
    bool close_to_target = std::abs(dx) < 50.0f;

    if (reached_target || close_to_target) {
      // Stop horizontal movement
      rigid->velocity.x = 0.0f;
      state_machine->change_state("Chasing");
      return;
    }

    // Safety: If dash takes too long (e.g., hit a wall), exit after 2 seconds
    float dash_time = Engine::Time::get_time() - Gamer_monkey->last_dash_time;
    if (dash_time > 2.0f) {
      rigid->velocity.x = 0.0f;
      state_machine->change_state("Chasing");
      return;
    }
  }

  void dashing_on_exit(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop movement when exiting dash
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Remove dash attack effect and damage layer
    if (entity.has_component<Components::Attack>()) {
      entity.remove_component<Components::Attack>();
    }
    entity.remove_from_layer("damage_player");

    Log::info("Gamer Monkey: Exited Dash state");
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================
  void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Gamer Monkey: Entered Chasing state");
  }

  void chasing_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();

    if (!Gamer_monkey) return;

    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid() && rigid && Gamer_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally using physics (force-based)
        if (distance > Gamer_monkey->close_attack_range) {
          float desired_vx = (dx > 0 ? Gamer_monkey->chase_speed : -Gamer_monkey->chase_speed);
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

        // If player moves out of normal chase detection but is still within
        // extended range, switch to dash windup for a long-range approach (if cooldown is ready)
        if (distance > Gamer_monkey->detection_range && distance <= Gamer_monkey->chase_range) {
          bool dash_ready = (current_time - Gamer_monkey->last_dash_time) >= Gamer_monkey->dash_cooldown;
          if (dash_ready) {
            state_machine->change_state("DashWindup");
            return;
          }
          // If dash is on cooldown, continue chasing normally
        }

        // Return to idle if player is completely out of range
        if (distance > Gamer_monkey->chase_range) {
          state_machine->change_state("Idle");
        }

        // If close enough and cooldown has elapsed, go to windup before attacking
        if (distance < Gamer_monkey->close_attack_range &&
          (current_time - Gamer_monkey->last_attack_time) >= Gamer_monkey->attack_cooldown) {
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
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when windup started (reuse last_attack_time)
    if (Gamer_monkey) {
      Gamer_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Gamer Monkey: Winding up for attack");
  }

  void windup_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!Gamer_monkey) return;

    float current_time = Engine::Time::get_time();
    float windup_elapsed = current_time - Gamer_monkey->last_attack_time;

    // Get player entity to face them during windup
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid()) {
      auto* Gamer_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (Gamer_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - Gamer_monkey_transform->position.x;
        // Update flip direction to face player during wind-up
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // After windup duration, attack
    if (windup_elapsed >= Gamer_monkey->attack_windup) {
      Gamer_monkey->last_attack_time = current_time;
      state_machine->change_state("Attacking");
    }
  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  void attacking_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    // auto* transform = entity.get_component<Components::Transform2D>();

    if (rigid && flip && Gamer_monkey && rigid->on_ground) {
      // Throw monkey forward based on facing direction
      rigid->velocity.x = (flip->value.x > 0 ? Gamer_monkey->leap_speed : -Gamer_monkey->leap_speed);
      // Add upward velocity for a leap effect
      rigid->velocity.y = -Gamer_monkey->leap_height; // Negative is up
    }

    // Make the monkey itself the hitbox
    if (Gamer_monkey && flip) {
      // Add Attack component to the monkey
      // Note: We use the monkey's own collider for the attack hitbox now
      entity.add_component<Components::Attack>(Gamer_monkey->damage);
      auto* attack = entity.get_component<Components::Attack>();

      if (attack) {
        // Apply knockback direction based on facing
        attack->knockback = Gamer_monkey->knockback;
        if (flip->value.x < 0) {
          attack->knockback.x = -attack->knockback.x;
        }
        // Knockback Y is always positive (up) in our config, but physics needs negative for up
        attack->knockback.y = -std::abs(attack->knockback.y);
      }

      // Add to damage_player layer so it collides with player
      // We KEEP the enemy layer so it still collides with ground
      entity.add_to_layer("damage_player");
    }

    Log::info("Gamer Monkey: Entered Attacking state");
  }


  void attacking_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // Transition back to chasing when attack animation completes
    if (animation && animation->has_ended()) {
      state_machine->change_state("Chasing");
    }
  }

  void attacking_on_exit(Engine::ECS::Entity entity) {
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    if (Gamer_monkey) {
      // Update last attack time for cooldown
      Gamer_monkey->last_attack_time = Engine::Time::get_time();
    }

    // Remove Attack component and damage_player layer
    if (entity.has_component<Components::Attack>()) {
      entity.remove_component<Components::Attack>();
    }
    entity.remove_from_layer("damage_player");
  }


  void hit_on_enter(Engine::ECS::Entity entity) {
    (void)entity; // Suppress unused parameter warning
    Log::info("Gamer Monkey: Stunned!");
  }

  void hit_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Gamer_monkey = entity.get_component<Components::GamerMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!Gamer_monkey) return;

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
    Log::info("Gamer Monkey: Died!");
  }

  void death_on_update(Engine::ECS::Entity entity) {
    auto* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      Log::info("Gamer Monkey: Destroyed after death animation.");
    }
  }

  void death_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    // Should not happen unless respawning
  }

} // namespace GamerMonkeyStateCallbacks

namespace GamerMonkeyStates
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

    Components::StateAudio attack_audio;
    attack_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-2_2_Attack.wav");
    attack_audio.loop = false;
    attack_audio.volume = 0.6f;
    attack_audio.stop_on_exit = false;

    Components::StateAudio hurt_audio;
    hurt_audio.sound = Engine::Resources::load_sound("sounds/SFX/Monkey/Monkey VO/Monkey-1_8_Hurt.wav");
    hurt_audio.loop = false;
    hurt_audio.volume = 0.6f;
    hurt_audio.stop_on_exit = false;

    // ========================================================================
    // Gamer Monkey Idle State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_idle_anim;
    Gamer_monkey_idle_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_idle_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_idle_anim.animation = Components::Animation(6, 0.2f);
    Gamer_monkey_idle_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_idle("Idle", Gamer_monkey_idle_anim);

    // ========================================================================
    // Gamer Monkey Dash State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_dashing_anim;
    Gamer_monkey_dashing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    // atk.png frames are 96x64; update atlas to match
    Gamer_monkey_dashing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 25);
    Gamer_monkey_dashing_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f });
    Gamer_monkey_dashing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_dashing("Dash", Gamer_monkey_dashing_anim, footstep_audio);

    // ========================================================================
    // Gamer Monkey Dash Windup State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_dash_windup_anim;
    Gamer_monkey_dash_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    Gamer_monkey_dash_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 14);
    Gamer_monkey_dash_windup_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 0.2f, 0.3f, 0.4f, 0.5f });
    Gamer_monkey_dash_windup_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_dash_windup("DashWindup", Gamer_monkey_dash_windup_anim);

    // ========================================================================
    // Gamer Monkey Chase State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_chasing_anim;
    Gamer_monkey_chasing_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_chasing_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_chasing_anim.animation = Components::Animation(6, 0.2f);
    Gamer_monkey_chasing_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_chasing("Chasing", Gamer_monkey_chasing_anim, footstep_audio);

    // ========================================================================
    // Gamer Monkey Windup State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_windup_anim;
    Gamer_monkey_windup_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    Gamer_monkey_windup_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 5);
    Gamer_monkey_windup_anim.animation = Components::Animation(5, 0.3f);
    Gamer_monkey_windup_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_windup("Windup", Gamer_monkey_windup_anim);

    // ========================================================================
    // Gamer Monkey Attacking State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_attacking_anim;
    Gamer_monkey_attacking_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/atk.png"));
    Gamer_monkey_attacking_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 25, 8);
    Gamer_monkey_attacking_anim.animation = Components::Animation({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.3f });
    Gamer_monkey_attacking_anim.scale = Math::Vec2D(ATTACK_WIDTH, ATTACK_HEIGHT);

    Components::State Gamer_monkey_attacking("Attacking", Gamer_monkey_attacking_anim, attack_audio);

    // ========================================================================
    // Gamer Monkey Hit State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_hit_anim;
    Gamer_monkey_hit_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_hit_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_hit_anim.animation = Components::Animation(6, 0.2f);  // Changed from Gamer_monkey_idle_anim
    //Gamer_monkey_hit_anim.animation = Components::Animation(std::initializer_list{ 2 }, { 0.4f });
    Gamer_monkey_hit_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_hit("Hit", Gamer_monkey_hit_anim, hurt_audio);

    // ========================================================================
    // Gamer Monkey Death State
    // ========================================================================
    Components::StateAnimation Gamer_monkey_death_anim;
    Gamer_monkey_death_anim.texture = Components::Texture(Engine::Resources::load_texture("images/GameMonkeySpriteSheets/walk.png"));
    Gamer_monkey_death_anim.texture_atlas = Components::TextureAtlas(64, 64, 1, 6, 6);
    Gamer_monkey_death_anim.animation = Components::Animation(6, 0.2f);
    Gamer_monkey_death_anim.scale = Math::Vec2D(WIDTH, HEIGHT);

    Components::State Gamer_monkey_death("Death", Gamer_monkey_death_anim);

    // ========================================================================
    // Build and return state machine
    // ========================================================================
    Components::StateMachine Gamer_monkey_sm;
    Gamer_monkey_sm.blueprint_id = "GamerMonkey";
    Gamer_monkey_sm.add_state(Gamer_monkey_idle);
    Gamer_monkey_sm.add_state(Gamer_monkey_dash_windup);
    Gamer_monkey_sm.add_state(Gamer_monkey_dashing);
    Gamer_monkey_sm.add_state(Gamer_monkey_chasing);
    Gamer_monkey_sm.add_state(Gamer_monkey_windup);
    Gamer_monkey_sm.add_state(Gamer_monkey_attacking);
    Gamer_monkey_sm.add_state(Gamer_monkey_hit);
    Gamer_monkey_sm.add_state(Gamer_monkey_death);
    Gamer_monkey_sm.set_initial_state("Idle");

    return Gamer_monkey_sm;
  }
} // namespace GamerMonkeyStates

// Register blueprint + all gamer monkey state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(GamerMonkey, "Idle",
  STATE("Idle", GamerMonkeyStateCallbacks::idle_on_enter, GamerMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("DashWindup", GamerMonkeyStateCallbacks::dash_windup_on_enter, GamerMonkeyStateCallbacks::dash_windup_on_update, GamerMonkeyStateCallbacks::dash_windup_on_exit)
  STATE("Dash", GamerMonkeyStateCallbacks::dashing_on_enter, GamerMonkeyStateCallbacks::dashing_on_update, GamerMonkeyStateCallbacks::dashing_on_exit)
  STATE("Chasing", GamerMonkeyStateCallbacks::chasing_on_enter, GamerMonkeyStateCallbacks::chasing_on_update, GamerMonkeyStateCallbacks::chasing_on_exit)
  STATE("Windup", GamerMonkeyStateCallbacks::windup_on_enter, GamerMonkeyStateCallbacks::windup_on_update, NO_CALLBACK)
  STATE("Attacking", GamerMonkeyStateCallbacks::attacking_on_enter, GamerMonkeyStateCallbacks::attacking_on_update, GamerMonkeyStateCallbacks::attacking_on_exit)
  STATE("Hit", GamerMonkeyStateCallbacks::hit_on_enter, GamerMonkeyStateCallbacks::hit_on_update, GamerMonkeyStateCallbacks::hit_on_exit)
  STATE("Death", GamerMonkeyStateCallbacks::death_on_enter, GamerMonkeyStateCallbacks::death_on_update, GamerMonkeyStateCallbacks::death_on_exit)
)

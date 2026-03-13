//-----------------------------------------------------------------------------------
// file:    SlingshotMonkeyStateCallbacks.cpp
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Slingshot Monkey state machine callback implementations (serializable)
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#include "pch.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace SlingshotMonkeyStateCallbacks
{
  inline Math::Vec2D get_slingshot_detection_center(
    Engine::ECS::Entity entity,
    const Components::SlingshotMonkey& slingshot_monkey,
    const Components::Transform2D& transform
  ) {
    auto* collider = entity.get_component<Components::BoxCollider>();
    if (collider) {
      return collider->aabb.pos;
    }

    return Math::Vec2D(
      transform.position.x,
      transform.position.y + slingshot_monkey.hitbox_offset_y
    );
  }

  inline float get_slingshot_detection_half_height(
    Engine::ECS::Entity entity,
    const Components::SlingshotMonkey& slingshot_monkey
  ) {
    auto* collider = entity.get_component<Components::BoxCollider>();
    if (collider) {
      return std::max(collider->height * 0.5f, 1.0f);
    }

    return std::max(slingshot_monkey.hitbox_height * 0.5f, 1.0f);
  }

  inline float get_slingshot_detection_enter_buffer(const Components::SlingshotMonkey& slingshot_monkey) {
    return std::max(12.0f, slingshot_monkey.hitbox_width * 0.1f);
  }

  inline float get_slingshot_detection_exit_buffer(const Components::SlingshotMonkey& slingshot_monkey) {
    return std::max(20.0f, slingshot_monkey.hitbox_width * 0.2f);
  }

  inline bool is_player_within_detection_box(
    Engine::ECS::Entity entity,
    const Components::SlingshotMonkey& slingshot_monkey,
    float distance_x,
    float delta_y,
    float horizontal_range
  ) {
    return distance_x <= horizontal_range && std::abs(delta_y) <= get_slingshot_detection_half_height(entity, slingshot_monkey);
  }

  inline bool get_player_detection_metrics(
    Engine::ECS::Entity entity,
    float& out_distance,
    float& out_dx,
    float& out_dy,
    Components::Transform2D*& out_monkey_transform,
    Components::Transform2D*& out_player_transform
  ) {
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (!player_entity.is_valid()) {
      return false;
    }

    auto* slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    out_monkey_transform = entity.get_component<Components::Transform2D>();
    out_player_transform = player_entity.get_component<Components::Transform2D>();
    if (!slingshot_monkey || !out_monkey_transform || !out_player_transform) {
      return false;
    }

    const Math::Vec2D detection_center = get_slingshot_detection_center(entity, *slingshot_monkey, *out_monkey_transform);

    out_dx = out_player_transform->position.x - detection_center.x;
    out_dy = out_player_transform->position.y - detection_center.y;
    out_distance = std::abs(out_dx);
    return true;
  }

  inline float get_slingshot_engage_range(const Components::SlingshotMonkey& slingshot_monkey) {
    return std::max(slingshot_monkey.detection_range,
      std::max(slingshot_monkey.close_attack_range, slingshot_monkey.dash_attack_range));
  }

  inline float clamp_dash_target_x(const Components::SlingshotMonkey& slingshot_monkey, float monkey_x, float target_x) {
    const float max_dash_distance = slingshot_monkey.dash_attack_range;
    return std::clamp(target_x, monkey_x - max_dash_distance, monkey_x + max_dash_distance);
  }

  // ========================================================================
  // Updates knockback from current propulsion velocity
  // ========================================================================
  inline void update_propulsion_knockback(Engine::ECS::Entity entity) {
    auto* attack = entity.get_component<Components::Attack>();
    auto* slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!attack || !slingshot_monkey) return;

    // Keep attack damage synced with SlingshotMonkey config
    attack->damage = slingshot_monkey->damage;

    const float base_x = std::abs(slingshot_monkey->knockback.x);
    const float base_y = std::abs(slingshot_monkey->knockback.y);

    float dir_x = 1.0f;
    float propulsion_x = 0.0f;
    float upward_propulsion = 0.0f;

    if (rigid) {
      propulsion_x = std::abs(rigid->velocity.x);
      if (std::abs(rigid->velocity.x) > 0.01f) {
        dir_x = (rigid->velocity.x >= 0.0f) ? 1.0f : -1.0f;
      }
      upward_propulsion = std::max(0.0f, -rigid->velocity.y);
    }
    else if (flip) {
      dir_x = (flip->value.x >= 0.0f) ? 1.0f : -1.0f;
    }

    // Horizontal knockback follows propulsion, but never weaker than base tuning.
    attack->knockback.x = dir_x * std::max(base_x, propulsion_x);
    // Add some upward contribution from current vertical propulsion.
    attack->knockback.y = -(base_y + upward_propulsion * 0.35f);
  }

  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop movement when entering idle
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    Log::info("Slingshot Monkey: Entered Idle state");
  }

  inline void idle_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();

    if (!Slingshot_monkey || !state_machine) return;

    float current_time = Engine::Time::get_time();

    float dx = 0.0f;
    float dy = 0.0f;
    float distance = 0.0f;
    Components::Transform2D* Slingshot_monkey_transform = nullptr;
    Components::Transform2D* player_transform = nullptr;
    if (get_player_detection_metrics(entity, distance, dx, dy, Slingshot_monkey_transform, player_transform)) {
      // Check if dash is ready
      bool dash_ready = (current_time - Slingshot_monkey->last_dash_time) >= Slingshot_monkey->dash_cooldown;
      const float detection_enter_range = std::max(0.0f,
        Slingshot_monkey->detection_range - get_slingshot_detection_enter_buffer(*Slingshot_monkey));

      if (is_player_within_detection_box(entity, *Slingshot_monkey, distance, dy, detection_enter_range)) {
        if (dash_ready) {
          state_machine->change_state("DashWindup");
        }
        else {
          state_machine->change_state("Chasing");
        }
      }
    }
  }

  // ========================================================================
  // Dash Windup State Callbacks
  // ========================================================================
  inline void dash_windup_on_enter(Engine::ECS::Entity entity) {
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    auto* Slingshot_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    if (!Slingshot_monkey || !Slingshot_monkey_transform) return;

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when dash windup started
    Slingshot_monkey->dash_windup_start_time = Engine::Time::get_time();

    // Get player position to determine direction
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    float monkey_x = Slingshot_monkey_transform->position.x;
    float player_x = monkey_x;

    if (player_entity.is_valid()) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();
      if (player_transform) {
        player_x = player_transform->position.x;

        // Face the player
        if (flip) {
          float dx = player_x - monkey_x;
          flip->value.x = (dx >= 0.0f ? 1.0f : -1.0f);
        }
      }
    }

    // Create the warning indicator rectangle entity
    float monkey_y = Slingshot_monkey_transform->position.y;
    float direction = (player_x >= monkey_x) ? 1.0f : -1.0f;

    // Start with a small width rectangle at the monkey's position
    float initial_width = 50.0f;
    float indicator_x = monkey_x + (direction * initial_width * 0.5f);

    // Create indicator entity with a colored rectangle (uses default white texture with color tint)
    Slingshot_monkey->dash_indicator_entity = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(
        indicator_x,
        monkey_y,
        0.0f,
        initial_width,
        Slingshot_monkey->dash_indicator_height
      )
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/white.png")) // Use a white texture for colored rect
      .add_component<Components::Color>(Math::Vec4D(1.0f, 0.3f, 0.3f, 0.5f)); // Semi-transparent red

    // Store initial direction for the indicator
    Slingshot_monkey->dash_target_x = clamp_dash_target_x(*Slingshot_monkey, monkey_x, player_x);

    Log::info("Slingshot Monkey: Entered Dash Windup state");
  }

  inline void dash_windup_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    auto* Slingshot_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!Slingshot_monkey || !Slingshot_monkey_transform || !state_machine) return;

    float current_time = Engine::Time::get_time();
    float elapsed = current_time - Slingshot_monkey->dash_windup_start_time;
    float progress = std::min(elapsed / Slingshot_monkey->dash_windup_duration, 1.0f);

    float monkey_x = Slingshot_monkey_transform->position.x;
    float monkey_y = Slingshot_monkey_transform->position.y;
    float target_x = Slingshot_monkey->dash_target_x;

    float direction = (target_x >= monkey_x) ? 1.0f : -1.0f;

    // Keep facing the stored dash target throughout the windup.
    if (flip) {
      flip->value.x = direction;
    }

    // Grow the warning toward the stored target for a committed windup.
    float distance_to_target = std::abs(target_x - monkey_x);

    float min_width = 50.0f;
    float target_width = distance_to_target + 50.0f; // Extend slightly past target
    float current_width = min_width + (target_width - min_width) * progress;

    // Update the warning indicator rectangle
    if (Slingshot_monkey->dash_indicator_entity.is_valid()) {
      auto* indicator_transform = Slingshot_monkey->dash_indicator_entity.get_component<Components::Transform2D>();
      if (indicator_transform) {
        // Position the rectangle so it starts at monkey and extends toward player
        float indicator_x = monkey_x + (direction * current_width * 0.5f);
        indicator_transform->position.x = indicator_x;
        indicator_transform->position.y = monkey_y;
        indicator_transform->scale.x = current_width;
        indicator_transform->scale.y = Slingshot_monkey->dash_indicator_height;
      }

      // Make the indicator flash/pulse by adjusting alpha and color intensity
      auto* indicator_color = Slingshot_monkey->dash_indicator_entity.get_component<Components::Color>();
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
      state_machine->change_state("Dash");
    }
  }

  inline void dash_windup_on_exit(Engine::ECS::Entity entity) {
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();

    // Destroy the warning indicator entity
    if (Slingshot_monkey && Slingshot_monkey->dash_indicator_entity.is_valid()) {
      Engine::ECS::destroy_entity(Slingshot_monkey->dash_indicator_entity);
      Slingshot_monkey->dash_indicator_entity = Engine::ECS::Entity(); // Reset to invalid
    }

    Log::info("Slingshot Monkey: Exited Dash Windup state");
  }

  // ========================================================================
  // Dashing State Callbacks
  // ========================================================================
  inline void dashing_on_enter(Engine::ECS::Entity entity) {
    auto* flip = entity.get_component<Components::Flip>();
    auto* Slingshot_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    if (!Slingshot_monkey) return;

    // Record when dash started (for cooldown tracking)
    Slingshot_monkey->last_dash_time = Engine::Time::get_time();

    // Calculate direction to dash target
    float dx = 0.0f;
    if (Slingshot_monkey_transform) {
      dx = Slingshot_monkey->dash_target_x - Slingshot_monkey_transform->position.x;
    }

    // Face toward the stored dash target position
    if (flip) {
      flip->value.x = (dx >= 0.0f ? 1.0f : -1.0f);
    }

    // Apply dash velocity toward the stored target X position
    if (rigid) {
      // Use a high dash speed to ensure reaching the target
      rigid->velocity.x = (dx >= 0.0f ? Slingshot_monkey->dash_speed : -Slingshot_monkey->dash_speed);
      // Small upward boost for visual effect
      rigid->velocity.y = -100.0f;
    }

    // Make the monkey itself deal damage on collision while dashing
    if (!entity.has_component<Components::Attack>()) {
      entity.add_component<Components::Attack>(Slingshot_monkey->damage);
    }
    auto* attack = entity.get_component<Components::Attack>();
    if (attack) {
      attack->damage = Slingshot_monkey->damage;
      attack->hit_entities.clear();
      update_propulsion_knockback(entity);
    }
    entity.add_to_layer("damage_player");

    // Note: Logging target X here previously used std::to_string in a header, which caused a compile-time
    // constant expression issue on some compilers. If needed, log from a .cpp or use a simpler message.
    Log::info("Slingshot Monkey: Entered Dash state");
  }

  inline void dashing_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Slingshot_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();

    if (!Slingshot_monkey || !state_machine) return;

    if (!rigid || !Slingshot_monkey_transform) {
      state_machine->change_state("Idle");
      return;
    }

    // Keep applying dash velocity (in case friction slows it down)
    float current_x = Slingshot_monkey_transform->position.x;
    float target_x = Slingshot_monkey->dash_target_x;
    float dx = target_x - current_x;

    // Maintain dash speed
    if (std::abs(dx) > 10.0f) {
      rigid->velocity.x = (dx >= 0.0f ? Slingshot_monkey->dash_speed : -Slingshot_monkey->dash_speed);
    }

    // Keep knockback synced to propulsion while dashing.
    update_propulsion_knockback(entity);

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
    float dash_time = Engine::Time::get_time() - Slingshot_monkey->last_dash_time;
    if (dash_time > 2.0f) {
      rigid->velocity.x = 0.0f;
      state_machine->change_state("Chasing");
      return;
    }
  }

  inline void dashing_on_exit(Engine::ECS::Entity entity) {
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

    Log::info("Slingshot Monkey: Exited Dash state");
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================
  inline void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Slingshot Monkey: Entered Chasing state");
  }

  inline void chasing_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Slingshot_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();

    if (!Slingshot_monkey) return;

    float current_time = Engine::Time::get_time();

    if (rigid && Slingshot_monkey_transform) {
      float dx = 0.0f;
      float dy = 0.0f;
      float distance = 0.0f;
      Components::Transform2D* player_transform = nullptr;
      Components::Transform2D* monkey_transform_unused = nullptr;

      if (get_player_detection_metrics(entity, distance, dx, dy, monkey_transform_unused, player_transform)) {
        float small_gap_threshold = Slingshot_monkey->close_attack_range;
        bool dash_ready = (current_time - Slingshot_monkey->last_dash_time) >= Slingshot_monkey->dash_cooldown;
        const float range_enter_buffer = get_slingshot_detection_enter_buffer(*Slingshot_monkey);
        const float dash_trigger_range = std::max(20.0f, Slingshot_monkey->dash_attack_range - range_enter_buffer);
        const float close_attack_trigger_range = std::max(0.0f, small_gap_threshold - range_enter_buffer);
        const float disengage_range = get_slingshot_engage_range(*Slingshot_monkey) +
          get_slingshot_detection_exit_buffer(*Slingshot_monkey);

        // If dash is back up, immediately swap back to dash attack.
        if (dash_ready && distance > 20.0f &&
          is_player_within_detection_box(entity, *Slingshot_monkey, distance, dy, dash_trigger_range)) {
          state_machine->change_state("DashWindup");
          return;
        }

        // While waiting for dash cooldown: if near enough, use close-range attack.
        if (!dash_ready &&
          is_player_within_detection_box(entity, *Slingshot_monkey, distance, dy, close_attack_trigger_range) &&
          (current_time - Slingshot_monkey->last_attack_time) >= Slingshot_monkey->attack_cooldown) {
          state_machine->change_state("Windup");
          return;
        }

        // Chase player horizontally using physics (force-based).
        // For small gaps, keep chasing with reduced speed to close the distance precisely.
        if (distance > 20.0f) {
          float chase_speed = (distance <= small_gap_threshold)
            ? (Slingshot_monkey->chase_speed * 0.6f)
            : Slingshot_monkey->chase_speed;

          float desired_vx = (dx > 0 ? chase_speed : -chase_speed);
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
          // Stop jittering when essentially touching target X
          rigid->velocity.x = 0.0f;
        }

        // Return to idle if player is completely out of range
        if (!is_player_within_detection_box(entity, *Slingshot_monkey, distance, dy, disengage_range)) {
          rigid->velocity.x = 0.0f;
          state_machine->change_state("Idle");
          return;
        }
      }
    }
  }

  inline void chasing_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {

  }

  //// ========================================================================
  //// Windup State Callbacks
  //// ========================================================================

  //inline void windup_on_enter(Engine::ECS::Entity entity) {
  //  auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
  //  auto* rigid = entity.get_component<Components::RigidBody>();

  //  // Stop moving during windup
  //  if (rigid) {
  //    rigid->velocity.x = 0.0f;
  //  }

  //  // Record when windup started (reuse last_attack_time)
  //  if (Slingshot_monkey) {
  //    Slingshot_monkey->last_attack_time = Engine::Time::get_time();
  //  }

  //  Log::info("Slingshot Monkey: Winding up for attack");
  //}

  //inline void windup_on_update(Engine::ECS::Entity entity) {
  //  auto* state_machine = entity.get_component<Components::StateMachine>();
  //  auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
  //  auto* flip = entity.get_component<Components::Flip>();

  //  if (!Slingshot_monkey) return;

  //  float current_time = Engine::Time::get_time();
  //  float windup_elapsed = current_time - Slingshot_monkey->last_attack_time;

  //  // Get player entity to face them during windup
  //  auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

  //  if (player_entity.is_valid()) {
  //    auto* Slingshot_monkey_transform = entity.get_component<Components::Transform2D>();
  //    auto* player_transform = player_entity.get_component<Components::Transform2D>();

  //    if (Slingshot_monkey_transform && player_transform && flip) {
  //      float dx = player_transform->position.x - Slingshot_monkey_transform->position.x;
  //      // Update flip direction to face player during wind-up
  //      flip->value.x = (dx > 0 ? 1.0f : -1.0f);
  //    }
  //  }

  //  // After windup duration, attack
  //  if (windup_elapsed >= Slingshot_monkey->attack_windup) {
  //    Slingshot_monkey->last_attack_time = current_time;
  //    state_machine->change_state("Attacking");
  //  }
  //}

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  inline void attacking_on_enter(Engine::ECS::Entity entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    // auto* transform = entity.get_component<Components::Transform2D>();

    if (rigid && flip && Slingshot_monkey && rigid->on_ground) {
      // Throw monkey forward based on facing direction
      rigid->velocity.x = (flip->value.x > 0 ? Slingshot_monkey->leap_speed : -Slingshot_monkey->leap_speed);
      // Add upward velocity for a leap effect
      rigid->velocity.y = -Slingshot_monkey->leap_height; // Negative is up
    }

    // Make the monkey itself the hitbox
    if (Slingshot_monkey && flip) {
      // Add Attack component to the monkey if missing
      // Note: We use the monkey's own collider for the attack hitbox now
      if (!entity.has_component<Components::Attack>()) {
        entity.add_component<Components::Attack>(Slingshot_monkey->damage);
      }
      auto* attack = entity.get_component<Components::Attack>();

      if (attack) {
        attack->damage = Slingshot_monkey->damage;
        attack->hit_entities.clear();
      }

      // Knockback follows current propulsion
      update_propulsion_knockback(entity);

      // Add to damage_player layer so it collides with player
      // We KEEP the enemy layer so it still collides with ground
      entity.add_to_layer("damage_player");
    }

    // Mark attack start time so we keep hitbox active briefly even if animation data is missing.
    if (Slingshot_monkey) {
      Slingshot_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Slingshot Monkey: Entered Attacking state");
  }


  inline void attacking_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();

    // Keep knockback synced to propulsion during the leap/contact window.
    update_propulsion_knockback(entity);

    // Keep attack active for a short minimum window, then transition back.
    float elapsed = 0.0f;
    if (Slingshot_monkey) {
      elapsed = Engine::Time::get_time() - Slingshot_monkey->last_attack_time;
    }
    bool animation_done = (animation == nullptr) || animation->has_ended();

    // Transition back to chasing when attack animation completes and minimum active window has passed.
    if (animation_done && elapsed >= 0.15f) {
      state_machine->change_state("Chasing");
    }
  }

  inline void attacking_on_exit(Engine::ECS::Entity entity) {
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    if (Slingshot_monkey) {
      // Update last attack time for cooldown
      Slingshot_monkey->last_attack_time = Engine::Time::get_time();
    }

    // Remove Attack component and damage_player layer
    if (entity.has_component<Components::Attack>()) {
      entity.remove_component<Components::Attack>();
    }
    entity.remove_from_layer("damage_player");
  }


  inline void hit_on_enter(Engine::ECS::Entity entity) {
    (void)entity; // Suppress unused parameter warning
    Log::info("Slingshot Monkey: Stunned!");
  }

  inline void hit_on_update(Engine::ECS::Entity entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Slingshot_monkey = entity.get_component<Components::SlingshotMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!Slingshot_monkey) return;

    // When animation ends, detach from hitbox and apply knockback
    if (animation->has_ended() && pending_knockback && rigid) {
      // Detach from hitbox first (stop following player)
      if (entity.has_component<Components::Parent>()) {
        entity.remove_component<Components::Parent>();
      }

      // Remove the pending knockback component (one-time use)
      entity.remove_component<Components::PendingKnockback>();
    }

    // Transition to idle only when grounded (after knockback has been applied)
    if (animation->has_ended() && rigid && rigid->on_ground) {
      state_machine->change_state("Idle");
    }
  }

  inline void hit_on_exit(Engine::ECS::Entity entity) {
    // Cleanup: ensure parent is removed (safety check)
    if (entity.has_component<Components::Parent>()) {
      entity.remove_component<Components::Parent>();
    }
  }

  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  inline void death_on_enter([[maybe_unused]] Engine::ECS::Entity entity) {
    Log::info("Slingshot Monkey: Died!");
  }

  inline void death_on_update(Engine::ECS::Entity entity) {
    auto* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      Log::info("Slingshot Monkey: Destroyed after death animation.");
    }
  }

  inline void death_on_exit([[maybe_unused]] Engine::ECS::Entity entity) {
    // Should not happen unless respawning
  }

} // namespace SlingshotMonkeyStateCallbacks

// Register blueprint + all Slingshot monkey state callbacks
REGISTER_STATE_MACHINE_BLUEPRINT(SlingshotMonkey, "Idle",
  STATE("Idle", SlingshotMonkeyStateCallbacks::idle_on_enter, SlingshotMonkeyStateCallbacks::idle_on_update, NO_CALLBACK)
  STATE("DashWindup", SlingshotMonkeyStateCallbacks::dash_windup_on_enter, SlingshotMonkeyStateCallbacks::dash_windup_on_update, SlingshotMonkeyStateCallbacks::dash_windup_on_exit)
  STATE("Dash", SlingshotMonkeyStateCallbacks::dashing_on_enter, SlingshotMonkeyStateCallbacks::dashing_on_update, SlingshotMonkeyStateCallbacks::dashing_on_exit)
  STATE("Chasing", SlingshotMonkeyStateCallbacks::chasing_on_enter, SlingshotMonkeyStateCallbacks::chasing_on_update, SlingshotMonkeyStateCallbacks::chasing_on_exit)
  //STATE("Windup", SlingshotMonkeyStateCallbacks::windup_on_enter, SlingshotMonkeyStateCallbacks::windup_on_update, NO_CALLBACK)
  STATE("Attacking", SlingshotMonkeyStateCallbacks::attacking_on_enter, SlingshotMonkeyStateCallbacks::attacking_on_update, SlingshotMonkeyStateCallbacks::attacking_on_exit)
  STATE("Hit", SlingshotMonkeyStateCallbacks::hit_on_enter, SlingshotMonkeyStateCallbacks::hit_on_update, SlingshotMonkeyStateCallbacks::hit_on_exit)
  STATE("Death", SlingshotMonkeyStateCallbacks::death_on_enter, SlingshotMonkeyStateCallbacks::death_on_update, SlingshotMonkeyStateCallbacks::death_on_exit)
)

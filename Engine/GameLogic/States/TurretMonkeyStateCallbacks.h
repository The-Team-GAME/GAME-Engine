//-----------------------------------------------------------------------------------
// file:    TurretMonkeyStateCallbacks.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang An Kiat (100%)
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Turret Monkey state machine callback implementations (serializable)
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"
#include "../../Engine/Engine/Systems/CollisionSystem.h"
#include "../Components/GameplayComponent.h"
#include "../Components/EnemyHealthComponent.h"

namespace TurretMonkeyStateCallbacks
{

  // ========================================================================
  // Bannana Projectile
  // ========================================================================

  inline void fire_turret_projectile(Engine::ECS::Entity& entity) {
    auto* flip = entity.get_component<Components::Flip>();
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    auto* transform = entity.get_component<Components::Transform2D>();

    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!Turret_monkey || !flip || !transform || !player_entity.is_valid()) {
      return;
    }

    // Compute spawn position slightly in front of monkey
    float offset_x = (flip->value.x < 0.0f) ? -Turret_monkey->hitbox_offset_x : Turret_monkey->hitbox_offset_x;
    float offset_y = Turret_monkey->hitbox_offset_y;

    float spawn_x = transform->position.x + offset_x;
    float spawn_y = transform->position.y + offset_y;

    // Decide horizontal direction based on facing
    float dir_x = (flip->value.x < 0.0f) ? -1.0f : 1.0f;
    const float projectile_speed = 300.0f;

    const float projectile_width = 70.0f;
    const float projectile_height = 70.0f;

    auto projectile = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(spawn_x, spawn_y, 0.0f, projectile_width, projectile_height)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f)
      .add_component<Components::Texture>(Engine::Resources::load_texture("images/banana-projectile.png"))
      .add_component<Components::RigidBody>(false, false) // Dynamic, no gravity for straight shot
      .add_component<Components::BoxCollider>(projectile_width, projectile_height, false)
      .add_component<Components::Attack>(Turret_monkey->damage)
      .add_component<Components::Lifetime>(2.0f) // Despawn after a short time
      .remove_from_layer("default")
      .add_to_layer("effect")
      .add_to_layer("damage_player");

    if (auto* proj_rigid = projectile.get_component<Components::RigidBody>()) {
      proj_rigid->velocity.x = dir_x * projectile_speed;
      proj_rigid->angular_velocity = 360.0f; // Constant spin
    }

    if (auto* attack = projectile.get_component<Components::Attack>()) {
      attack->knockback = Turret_monkey->knockback;
      if (dir_x < 0.0f) {
        attack->knockback.x = -attack->knockback.x;
      }
      attack->knockback.y = -std::abs(attack->knockback.y);
    }
  }

  
  // ========================================================================
  // Ceiling Snapper
  // ========================================================================

  inline bool is_touching_ceiling(Engine::ECS::Entity& entity) {
    auto* monkey_collider = entity.get_component<Components::BoxCollider>();
    if (!monkey_collider) {
      return false;
    }

    bool touching_ceiling = false;

    // Scan all tile colliders in the world and see if any overlap the monkey
    Engine::ECS::for_each_entity_with<Components::TileCollider, Components::BoxCollider>(
      [&](Engine::ECS::Entity tile_entity) {
        if (touching_ceiling) {
          return;
        }

        auto* tile_collider = tile_entity.get_component<Components::BoxCollider>();
        if (!tile_collider) {
          return;
        }

        // Basic AABB overlap check
        if (!Engine::Systems::Collision2DSystem::rect_rect_coll(*monkey_collider, *tile_collider)) {
          return;
        }

        // Treat only tiles whose center is above the monkey as "ceiling" tiles
        if (tile_collider->aabb.pos.y < monkey_collider->aabb.pos.y) {
          touching_ceiling = true;
        }
      });

    return touching_ceiling;
  }

  inline bool try_snap_to_nearby_ceiling(Engine::ECS::Entity& entity) {
    auto* transform = entity.get_component<Components::Transform2D>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* box = entity.get_component<Components::BoxCollider>();
    auto* state_machine = entity.get_component<Components::StateMachine>();

    if (!transform || !rigid || !box || !state_machine) {
      return false;
    }

    const float snap_vertical_threshold = 20.0f;   // how close to ceiling before snapping
    const float snap_horizontal_margin = 5.0f;    // small extra horizontal leeway

    Components::BoxCollider* best_tile = nullptr;
    float best_gap = snap_vertical_threshold;

    float monkey_half_w = box->width * 0.5f;
    float monkey_half_h = box->height * 0.5f;
    float monkey_center_x = transform->position.x;
    float monkey_top = transform->position.y - monkey_half_h;

    Engine::ECS::for_each_entity_with<Components::TileCollider, Components::BoxCollider>(
      [&](Engine::ECS::Entity tile_entity) {
        auto* tile_collider = tile_entity.get_component<Components::BoxCollider>();
        if (!tile_collider) {
          return;
        }

        float tile_half_w = tile_collider->aabb.half_width;
        float tile_half_h = tile_collider->aabb.half_height;
        float tile_center_x = tile_collider->aabb.pos.x;
        float tile_center_y = tile_collider->aabb.pos.y;

        // Only consider tiles above the monkey
        if (tile_center_y >= transform->position.y) {
          return;
        }

        // Require reasonable horizontal alignment under the tile
        float horizontal_dist = std::abs(tile_center_x - monkey_center_x);
        float allowed_horizontal = tile_half_w + monkey_half_w + snap_horizontal_margin;
        if (horizontal_dist > allowed_horizontal) {
          return;
        }

        // Distance between monkey top and tile bottom
        float tile_bottom = tile_center_y + tile_half_h;
        float gap = std::abs(tile_bottom - monkey_top);
        if (gap < best_gap) {
          best_gap = gap;
          best_tile = tile_collider;
        }
      });

    if (!best_tile) {
      return false;
    }

    // Snap monkey so its top touches the tile's bottom
    float tile_bottom = best_tile->aabb.pos.y + best_tile->aabb.half_height;
    float new_center_y = tile_bottom + monkey_half_h;
    transform->position.y = new_center_y;

    rigid->velocity.zero();
    rigid->use_gravity = false;
    state_machine->change_state("Turret");
    return true;
  }

  // ========================================================================
  // Idle State Callbacks
  // ========================================================================

  inline void idle_on_enter(Engine::ECS::Entity& entity) {
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    if (Turret_monkey) {
      // Store when we entered idle (for recovery delay after being hit)
      Turret_monkey->last_attack_time = Engine::Time::get_time(); // Reusing this field as "idle_enter_time"
    }
    Log::info("Turret Monkey: Entered Idle state");
  }

  inline void idle_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();

    if (!Turret_monkey) return;

    float current_time = Engine::Time::get_time();
    float time_in_idle = current_time - Turret_monkey->last_attack_time;

    // Don't chase immediately after entering idle (recovery from hit)
    if (time_in_idle < 0.3f) {
      return;
    }

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (!player_entity.is_valid()) {
      return;
    }

    auto* Turret_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* player_transform = player_entity.get_component<Components::Transform2D>();

    if (Turret_monkey_transform && player_transform) {
      float dx = player_transform->position.x - Turret_monkey_transform->position.x;
      float dy = player_transform->position.y - Turret_monkey_transform->position.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      // Start chasing if player is within detection range
      if (distance < Turret_monkey->detection_range) {
        state_machine->change_state("Chasing");
      }
    }
  }

  // ========================================================================
  // Chasing State Callbacks
  // ========================================================================

  inline void chasing_on_enter([[maybe_unused]] Engine::ECS::Entity& entity) {
    Log::info("Turret Monkey: Entered Chasing state");
  }

  inline void chasing_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Turret_monkey_transform = entity.get_component<Components::Transform2D>();
    auto* flip = entity.get_component<Components::Flip>();
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();

    if (!Turret_monkey) return;

    // If we're close enough under a ceiling, snap up and become a turret
    if (try_snap_to_nearby_ceiling(entity)) {
      return;
    }

    float current_time = Engine::Time::get_time();

    // Get player entity
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid() && rigid && Turret_monkey_transform) {
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (player_transform) {
        float dx = player_transform->position.x - Turret_monkey_transform->position.x;
        float distance = std::abs(dx); // Only horizontal distance for sidescrolling

        // Chase player horizontally, but stop when within attack range
        if (distance > Turret_monkey->long_attack_range) {
          rigid->velocity.x = (dx > 0 ? Turret_monkey->chase_speed : -Turret_monkey->chase_speed);
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
        if (distance > Turret_monkey->detection_range) {
          state_machine->change_state("Idle");
        }

        // If close enough and cooldown has elapsed, go to windup before attacking
        if (distance < Turret_monkey->long_attack_range &&
          (current_time - Turret_monkey->last_attack_time) >= Turret_monkey->attack_cooldown) {
          state_machine->change_state("Windup");
        }
      }
    }
  }

  inline void chasing_on_exit([[maybe_unused]] Engine::ECS::Entity& entity) {

  }

  // ========================================================================
  // Windup State Callbacks
  // ========================================================================

  inline void windup_on_enter(Engine::ECS::Entity& entity) {
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving during windup
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Record when windup started (reuse last_attack_time)
    if (Turret_monkey) {
      Turret_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Turret Monkey: Winding up for attack");
  }

  inline void windup_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!Turret_monkey) return;

    float current_time = Engine::Time::get_time();
    float windup_elapsed = current_time - Turret_monkey->last_attack_time;

    // Get player entity to face them during windup
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();

    if (player_entity.is_valid()) {
      auto* Turret_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (Turret_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - Turret_monkey_transform->position.x;
        // Update flip direction to face player during wind-up
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // After windup duration, attack
    if (windup_elapsed >= Turret_monkey->attack_windup) {
      Turret_monkey->last_attack_time = current_time;
      state_machine->change_state("Attacking");
    }
  }

  // ========================================================================
  // Turret (Stuck to Wall) State Callbacks
  // ========================================================================

  inline void turret_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();

    // Freeze movement and disable gravity while stuck
    if (rigid) {
      rigid->velocity.zero();
      rigid->use_gravity = false;
    }

    if (Turret_monkey) {
      Turret_monkey->last_attack_time = Engine::Time::get_time();
    }

    Log::info("Turret Monkey: Entered Turret state (stuck to wall)");
  }

  inline void turret_on_update(Engine::ECS::Entity& entity) {
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    auto* flip = entity.get_component<Components::Flip>();

    if (!Turret_monkey) return;

    float current_time = Engine::Time::get_time();

    // Face the player while stuck
    auto player_entity = Engine::ECS::get_first_entity_with<Components::Player>();
    if (player_entity.is_valid()) {
      auto* Turret_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (Turret_monkey_transform && player_transform && flip) {
        float dx = player_transform->position.x - Turret_monkey_transform->position.x;
        flip->value.x = (dx > 0 ? 1.0f : -1.0f);
      }
    }

    // Auto-fire projectiles on cooldown while in turret state
    if (player_entity.is_valid()) {
      auto* Turret_monkey_transform = entity.get_component<Components::Transform2D>();
      auto* player_transform = player_entity.get_component<Components::Transform2D>();

      if (Turret_monkey_transform && player_transform) {
        float dx = player_transform->position.x - Turret_monkey_transform->position.x;
        float distance = std::abs(dx);

        if (distance < Turret_monkey->long_attack_range &&
          (current_time - Turret_monkey->last_attack_time) >= Turret_monkey->attack_cooldown) {
          Turret_monkey->last_attack_time = current_time;
          fire_turret_projectile(entity);
        }
      }
    }
  }

  inline void turret_on_exit(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      rigid->use_gravity = true;
    }
    Log::info("Turret Monkey: Exited Turret state (no longer stuck)");
  }

  // ========================================================================
  // Attacking State Callbacks
  // ========================================================================

  inline void attacking_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();

    // Stop moving while firing
    if (rigid) {
      rigid->velocity.x = 0.0f;
    }

    // Fire a long-range projectile towards the player
    fire_turret_projectile(entity);

    Log::info("Turret Monkey: Entered Attacking state (firing projectile)");
  }


  inline void attacking_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* animation = entity.get_component<Components::Animation>();

    // Transition back to chasing when attack animation completes
    if (animation && animation->has_ended()) {
      state_machine->change_state("Chasing");
    }
  }

  inline void attacking_on_exit(Engine::ECS::Entity& entity) {
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    if (Turret_monkey) {
      // Update last attack time for cooldown
      Turret_monkey->last_attack_time = Engine::Time::get_time();
    }

    // Remove Attack component and damage_player layer
    if (entity.has_component<Components::Attack>()) {
      entity.remove_component<Components::Attack>();
    }
    entity.remove_from_layer("damage_player");
  }


  inline void hit_on_enter(Engine::ECS::Entity& entity) {
    auto* rigid = entity.get_component<Components::RigidBody>();
    if (rigid) {
      // Ensure gravity is turned back on when we get hit
      rigid->use_gravity = true;
    }
    Log::info("tURRET Monkey: Stunned!");
  }

  inline void hit_on_update(Engine::ECS::Entity& entity) {
    auto* state_machine = entity.get_component<Components::StateMachine>();
    auto* Turret_monkey = entity.get_component<Components::TurretMonkey>();
    auto* rigid = entity.get_component<Components::RigidBody>();
    auto* animation = entity.get_component<Components::Animation>();
    auto* pending_knockback = entity.get_component<Components::PendingKnockback>();

    if (!Turret_monkey) return;

    // When animation ends, detach from hitbox and apply knockback
    if (animation->has_ended() && pending_knockback && rigid) {
      // Detach from hitbox first (stop following player)
      if (entity.has_component<Components::Parent>()) {
        entity.remove_component<Components::Parent>();
      }

      // Apply knockback momentum from Player struct, scaled by TurretMonkey mass
      float mass = 20.0f; // Reference mass
      if (Turret_monkey) {
        mass = Turret_monkey->mass;
      }
      if (mass < 1.0f) {
        mass = 1.0f;
      }

      const float reference_mass = 20.0f;
      float scale = reference_mass / mass;
      Math::Vec2D scaled_knockback = pending_knockback->knockback * scale;

      rigid->velocity = scaled_knockback;

      // Remove the pending knockback component (one-time use)
      entity.remove_component<Components::PendingKnockback>();
    }

    // Transition to idle only when grounded (after knockback has been applied)
    if (animation->has_ended() && rigid && rigid->on_ground) {
      state_machine->change_state("Idle");
    }
  }

  inline void hit_on_exit(Engine::ECS::Entity& entity) {
    // removes paretn component on exit
    if (entity.has_component<Components::Parent>()) {
      entity.remove_component<Components::Parent>();
    }
  }

  // ========================================================================
  // Death State Callbacks
  // ========================================================================

  inline void death_on_enter([[maybe_unused]] Engine::ECS::Entity& entity) {
    Log::info("Turret Monkey: Died!");
  }

  inline void death_on_update(Engine::ECS::Entity& entity) {
    auto* animation = entity.get_component<Components::Animation>();
    if (animation && animation->has_ended()) {
      Engine::ECS::destroy_entity(entity);
      Log::info("Turret Monkey: Destroyed after death animation.");
    }
  }

  inline void death_on_exit([[maybe_unused]] Engine::ECS::Entity& entity) {
    // Should not happen unless respawning
  }

} // namespace TurretMonkeyStateCallbacks

// Register all Turret monkey state callbacks
REGISTER_FUNCTION(Turret_monkey_idle_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::idle_on_enter)
REGISTER_FUNCTION(Turret_monkey_idle_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::idle_on_update)
REGISTER_FUNCTION(Turret_monkey_chasing_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::chasing_on_enter)
REGISTER_FUNCTION(Turret_monkey_chasing_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::chasing_on_update)
REGISTER_FUNCTION(Turret_monkey_chasing_exit, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::chasing_on_exit)
REGISTER_FUNCTION(Turret_monkey_windup_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::windup_on_enter)
REGISTER_FUNCTION(Turret_monkey_windup_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::windup_on_update)
REGISTER_FUNCTION(Turret_monkey_attacking_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::attacking_on_enter)
REGISTER_FUNCTION(Turret_monkey_attacking_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::attacking_on_update)
REGISTER_FUNCTION(Turret_monkey_attacking_exit, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::attacking_on_exit)
REGISTER_FUNCTION(Turret_monkey_turret_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::turret_on_enter)
REGISTER_FUNCTION(Turret_monkey_turret_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::turret_on_update)
REGISTER_FUNCTION(Turret_monkey_turret_exit, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::turret_on_exit)
REGISTER_FUNCTION(Turret_monkey_hit_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::hit_on_enter)
REGISTER_FUNCTION(Turret_monkey_hit_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::hit_on_update)
REGISTER_FUNCTION(Turret_monkey_hit_exit, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::hit_on_exit)
REGISTER_FUNCTION(Turret_monkey_death_enter, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::death_on_enter)
REGISTER_FUNCTION(Turret_monkey_death_update, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::death_on_update)
REGISTER_FUNCTION(Turret_monkey_death_exit, void(Engine::ECS::Entity&), TurretMonkeyStateCallbacks::death_on_exit)

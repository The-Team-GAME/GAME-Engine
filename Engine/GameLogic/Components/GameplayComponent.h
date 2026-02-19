//-----------------------------------------------------------------------------------
// file:    GameplayComponent.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (70%)
//          Primary author
//          Cheng Ming Xian (30%)
//          Co-Author
// 
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines gameplay-related component structs, including Player (with AttackData), Enemy, LazyMonkey, Attack, and PendingKnockback, which store data for game mechanics.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

namespace Components
{
  struct Action
  {
    f32 duration;
    f32 timer;
    f32 cooldown;
    f32 cd_timer;

    f32 speed;
    f32 impulse;

    Action(f32 dur, f32 cd, f32 spd = 0.f, f32 imp = 0.f)
      : duration(dur), cooldown(cd), speed(spd), impulse(imp) {}

    void set_timer()
    {
      timer = duration;
    }

    void reset_cooldown()
    {
      cd_timer = cooldown;
    }

    bool timer_end() const
    {
      return timer <= 0.f;
    }
    
    bool ready() const
    {
      return cd_timer <= 0;
    }
  };

  struct Player
  {
    struct AttackData
    {
      f32 hitbox_offset_x = 80.0f;
      f32 hitbox_offset_y = 0.0f;
      f32 hitbox_width = 100.0f;
      f32 hitbox_height = 128.0f;
      f32 dash_speed = 1000.0f;
      s32 damage = 10;
      f32 heartrate_cost = 5.0f;
      Math::Vec2D knockback = Math::Vec2D(300.0f, 0.f);
    };


    //mass
    float mass = 50.0f;

    // Movement speeds (going to add to force instead of velocity as of 22/01/26)
    // These values are loaded from file
    f32 walk_speed = 300'000.f;
    f32 jump_force = -30'000.f;

    bool damage_immune = false;

    Action dash{ 0.2f,5.f, 1250.f };
    Action stunned{ 1.f, 0.f, 0.f, -5000.f };
    Action uppercut{ 1.f, 5.f, 500.f, 0 };
    Action smash{ 0.f, 7.f, 500.f, 0.f };

    // Attacks
    AttackData attack1 = {
      80.0f,                       // hitbox_offset_x
      0.0f,                        // hitbox_offset_y
      100.0f,                      // hitbox_width
      128.0f,                      // hitbox_height
      1000.0f,                     // dash_speed
      1,                          // damage
      5.0f,                        // heartrate_cost
      Math::Vec2D(300.0f, 0.f)     // knockback
    };

    AttackData attack2 = {
      80.0f,                       // hitbox_offset_x
      0.0f,                        // hitbox_offset_y
      100.0f,                      // hitbox_width
      128.0f,                      // hitbox_height
      1000.0f,                     // dash_speed
      1,                          // damage
      5.0f,                        // heartrate_cost
      Math::Vec2D(300.0f, 0.f)     // knockback
    };

    AttackData attack3 = {
      80.0f,                       // hitbox_offset_x
      0.0f,                        // hitbox_offset_y
      120.0f,                      // hitbox_width
      128.0f,                      // hitbox_height
      2000.0f,                     // dash_speed
      3,                           // damage
      8.0f,                        // heartrate_cost
      Math::Vec2D(3000.0f, 0.f)    // knockback
    };
  };

  struct Enemy
  {
  };

  struct SprayCanMonkey
  {
    // Combat stats
    float chase_speed = 80.0f;

    // Attack parameters
    float attack_cooldown = 5.0f;
    float last_attack_time = 0.0f;
    float attack_windup = 0.5f;  // Time to wait in windup state before attacking

    // Detection ranges
    float detection_range = 1200.0f;
    float chase_range = 1000.0f;
    float attack_range = 500.0f;

    // Spray-specific properties
    float spray_duration = 4.0f;
    float spray_range = 150.0f;
    float spray_width = 80.0f;

    float cloud_spawn_delay = 1.1f;     // spawn delay
    bool cloud_spawned = false;

    Engine::ECS::Entity cloud_entity{};
    float cloud_spawn_time = 0.f;
    float cloud_initial_vx = 0.f;

    // Knockback
    Math::Vec2D knockback = { 300.0f, 400.0f };

    // Damage
    int damage = 20;

  };

  struct LazyMonkey
  {
    // Movement speeds
    float chase_speed = 100.0f;

    // Attack parameters
    float leap_speed = 2000.0f;
    float leap_height = 300.0f;
    float attack_cooldown = 3.0f;
    float last_attack_time = 0.0f;
    float attack_windup = 0.5f;  // Time to wait in windup state before attacking

    // Attack hitbox parameters
    float hitbox_offset_x = 80.0f;
    float hitbox_offset_y = 0.0f;
    float hitbox_width = 120.0f;
    float hitbox_height = 128.0f;
    int damage = 10;
    Math::Vec2D knockback = Math::Vec2D(500.0f, 0.f);

    // Detection ranges
    float detection_range = 300.0f;
    float chase_range = 400.0f;
    float attack_range = 180.0f;

  };

  struct GamerMonkey
  {
    // Movement speeds
    float chase_speed = 100.0f;

    // Dash parameters (serialized/configurable)
    float dash_speed = 1000.0f;
    float dash_cooldown = 3.0f;      // Cooldown time before dash can be used again
    float dash_windup_duration = 1.0f;  // Time for the warning rectangle to extend
    float dash_indicator_height = 100.0f; // Height of the warning rectangle

    // Attack parameters (serialized/configurable)
    float leap_speed = 200.0f;
    float leap_height = 50.0f;
    float attack_cooldown = 3.0f;
    float attack_windup = 0.5f;  // Time to wait in windup state before attacking

    // Attack hitbox parameters
    float hitbox_offset_x = 80.0f;
    float hitbox_offset_y = 0.0f;
    float hitbox_width = 120.0f;
    float hitbox_height = 128.0f;
    int damage = 15;
    Math::Vec2D knockback = Math::Vec2D(500.0f, -100.0f);

    // Detection ranges
    float detection_range = 600.0f;
    float chase_range = 700.0f;
    float close_attack_range = 180.0f;
    float dash_attack_range = 400.0f;

    // Runtime state (NOT serialized - exceed 20 field limit)
    float last_dash_time = -3.0f;        // Initialize to allow immediate first dash
    float last_attack_time = 0.0f;       // Last attack timestamp
    float dash_windup_start_time = 0.0f; // When dash windup started
    float dash_target_x = 0.0f;          // The player's X position when dash windup completes
    Engine::ECS::Entity dash_indicator_entity; // Runtime reference to the warning indicator entity
  };

  struct TurretMonkey
  {
    // Movement speeds
    float chase_speed = 100.0f;

    // Attack parameters (serialized/configurable)
    float attack_cooldown = 3.0f;
    float attack_windup = 0.5f;  // Time to wait in windup state before attacking
    float last_attack_time = 0.0f;

    // Attack hitbox parameters
    float hitbox_offset_x = 80.0f;
    float hitbox_offset_y = 0.0f;
    float hitbox_width = 120.0f;
    float hitbox_height = 128.0f;
    int damage = 15;
    Math::Vec2D knockback = Math::Vec2D(500.0f, -100.0f);

    // Detection ranges
    float detection_range = 1200.0f;
    float long_attack_range = 600.0f;
  };

  struct Attack
  {
    int damage = 10;
    Math::Vec2D knockback = Math::Vec2D(0.0f, 0.0f);
    bool is_continuous = false;
    float damage_interval = 0.1f; // Time in seconds between damage ticks for continuous attacks
    std::vector<std::pair<Engine::ECS::Entity, float>> hit_entities; // Track entity and last hit time
    Components::Texture impact_vfx_texture;

    Attack() = default;
    explicit Attack(int dmg, Math::Vec2D kb = Math::Vec2D(0.0f, 0.0f), bool continuous = false, float interval = 0.1f)
      : damage(dmg), knockback(kb), is_continuous(continuous), damage_interval(interval) {
    }

    bool should_apply_damage(Engine::ECS::Entity entity) {
      float current_time = Engine::Time::get_time();

      // Check if this entity has already been hit
      auto it = std::find_if(hit_entities.begin(), hit_entities.end(),
        [entity](const std::pair<Engine::ECS::Entity, float>& pair) {
          return pair.first == entity;
        });

      if (it != hit_entities.end()) {
        // Entity found in hit list
        if (!is_continuous) {
          // Single hit attack, already hit
          return false;
        }
        else {
          // Continuous attack, check interval
          if (current_time - it->second >= damage_interval) {
            // Interval passed, update time and allow damage
            it->second = current_time;
            return true;
          }
          else {
            // Too soon
            return false;
          }
        }
      }

      // Entity not found, add it
      hit_entities.push_back({ entity, current_time });
      return true;
    }
  };

  struct PendingKnockback
  {
    Math::Vec2D knockback;
  };

} // namespace Components



REFLECT_FIELDS(Components::Player::AttackData, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, dash_speed, damage, heartrate_cost, knockback)
REFLECT_FIELDS(Components::Action, duration, timer, cooldown, cd_timer, speed, impulse)


REGISTER_COMPONENT(Components::Player, walk_speed, jump_force, dash, attack1, attack2, attack3)
REGISTER_COMPONENT(Components::LazyMonkey, chase_speed, leap_speed, leap_height, attack_cooldown, last_attack_time, attack_windup, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, damage, knockback, detection_range, chase_range, attack_range)
REGISTER_COMPONENT(Components::TurretMonkey, chase_speed, attack_cooldown, attack_windup, last_attack_time, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, damage, knockback, detection_range, long_attack_range)
REGISTER_COMPONENT(Components::GamerMonkey, chase_speed, dash_speed, dash_cooldown, dash_windup_duration, dash_indicator_height, leap_speed, leap_height, attack_cooldown, attack_windup, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, damage, knockback, detection_range, chase_range, close_attack_range, dash_attack_range)
REGISTER_COMPONENT(Components::SprayCanMonkey, chase_speed, attack_cooldown, last_attack_time, attack_windup, detection_range, chase_range, attack_range, spray_duration, spray_range, spray_width, knockback, damage, cloud_spawn_time, cloud_initial_vx)
REGISTER_COMPONENT(Components::Attack, damage, knockback, is_continuous, damage_interval, impact_vfx_texture)
REGISTER_COMPONENT(Components::PendingKnockback, knockback)
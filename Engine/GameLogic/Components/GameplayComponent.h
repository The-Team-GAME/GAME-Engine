//-----------------------------------------------------------------------------------
// file:    GameplayComponent.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines gameplay-related component structs, including Player (with AttackData), Enemy, LazyMonkey, Attack, and PendingKnockback, which store data for game mechanics.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/ECS/Macros.h"

#include "../../Engine/Engine/Time.h"

#include "../../Engine/Components/GraphicsComponents.h"

namespace Components
{

  struct Player
  {
    struct AttackData
    {
      float hitbox_offset_x = 80.0f;
      float hitbox_offset_y = 0.0f;
      float hitbox_width = 100.0f;
      float hitbox_height = 128.0f;
      float dash_speed = 1000.0f;
      int damage = 10;
      float heartrate_cost = 5.0f;
      Math::Vec2D knockback = Math::Vec2D(300.0f, -50.0f);
    };

    // Movement speeds
    float walk_speed = 600.0f;
    float air_speed = 600.0f;
    float jump_force = 900.0f;

    // Attacks
    AttackData attack1 = {
      80.0f,                       // hitbox_offset_x
      0.0f,                        // hitbox_offset_y
      100.0f,                      // hitbox_width
      128.0f,                      // hitbox_height
      1000.0f,                     // dash_speed
      10,                          // damage
      5.0f,                        // heartrate_cost
      Math::Vec2D(300.0f, -50.0f)  // knockback
    };

    AttackData attack2 = {
      80.0f,                       // hitbox_offset_x
      0.0f,                        // hitbox_offset_y
      100.0f,                      // hitbox_width
      128.0f,                      // hitbox_height
      1000.0f,                     // dash_speed
      15,                          // damage
      5.0f,                        // heartrate_cost
      Math::Vec2D(300.0f, -50.0f)  // knockback
    };

    AttackData attack3 = {
      80.0f,                       // hitbox_offset_x
      0.0f,                        // hitbox_offset_y
      120.0f,                      // hitbox_width
      128.0f,                      // hitbox_height
      2000.0f,                     // dash_speed
      25,                          // damage
      8.0f,                        // heartrate_cost
      Math::Vec2D(3000.0f, -100.0f)// knockback
    };
  };

  struct Enemy
  {
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
    int damage = 15;
    Math::Vec2D knockback = Math::Vec2D(500.0f, -100.0f);

    // Detection ranges
    float detection_range = 300.0f;
    float chase_range = 400.0f;
    float attack_range = 180.0f;
  };

  struct GamerMonkey
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
    int damage = 15;
    Math::Vec2D knockback = Math::Vec2D(500.0f, -100.0f);

    // Detection ranges
    float detection_range = 600.0f;
    float chase_range = 700.0f;
    float close_attack_range = 180.0f;
		float long_attack_range = 400.0f;
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
      : damage(dmg), knockback(kb), is_continuous(continuous), damage_interval(interval) {}

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
REGISTER_COMPONENT(Components::Player, walk_speed, air_speed, jump_force, attack1, attack2, attack3)
REGISTER_COMPONENT(Components::LazyMonkey, chase_speed, leap_speed, leap_height, attack_cooldown, last_attack_time, attack_windup, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, damage, knockback, detection_range, chase_range, attack_range)
REGISTER_COMPONENT(Components::GamerMonkey, chase_speed, leap_speed, leap_height, attack_cooldown, last_attack_time, attack_windup, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, damage, knockback, detection_range, chase_range, close_attack_range, long_attack_range)
REGISTER_COMPONENT(Components::Attack, damage, knockback, is_continuous, damage_interval, impact_vfx_texture)
REGISTER_COMPONENT(Components::PendingKnockback, knockback)
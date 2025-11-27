#pragma once

#include "../../Engine/ECS/Macros.h"

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
    float air_speed = 150.0f;
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
    float leap_speed = 400.0f;
    float leap_height = 300.0f;
    float attack_cooldown = 5.0f;
    float last_attack_time = 0.0f;

    // Detection ranges
    float detection_range = 300.0f;
    float chase_range = 400.0f;
    float attack_range = 80.0f;
  };

  struct Attack
  {
    int damage = 10;
    Math::Vec2D knockback = Math::Vec2D(0.0f, 0.0f);
    std::vector<Engine::ECS::Entity> hit_entities; // Track which entities have been hit by this attack

    Attack() = default;
    explicit Attack(int dmg, Math::Vec2D kb = Math::Vec2D(0.0f, 0.0f)) : damage(dmg), knockback(kb) {}
  };

  struct PendingKnockback
  {
    Math::Vec2D knockback;
  };

} // namespace Components















REFLECT_FIELDS(Components::Player::AttackData, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height, dash_speed, damage, heartrate_cost, knockback)
REGISTER_COMPONENT(Components::Player, walk_speed, air_speed, jump_force, attack1, attack2, attack3)
REGISTER_COMPONENT(Components::LazyMonkey, chase_speed, leap_speed, leap_height, attack_cooldown, last_attack_time, detection_range, chase_range, attack_range)
REGISTER_COMPONENT(Components::Attack, damage, knockback)
REGISTER_COMPONENT(Components::PendingKnockback, knockback)
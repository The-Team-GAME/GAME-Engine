#pragma once

#include "../../Engine/ECS/Macros.h"

namespace Components
{
  struct Player
  {
    // Movement speeds
    float walk_speed = 600.0f;
    float air_speed = 150.0f;
    float jump_force = 900.0f;

    // Attack parameters
    float hitbox_offset = 80.0f;
    float hitbox_width = 100.0f;
    float hitbox_height = 128.0f; // ATTACK_HEIGHT * 2.0f
    float attack_dash_speed1 = 3500.0f; // First punch forward movement
    float attack_dash_speed2 = 3500.0f; // Second punch forward movement
    float attack_dash_speed3 = 5000.0f; // Third punch (finisher) forward movement
    int attack_damage1 = 10; // First punch damage
    int attack_damage2 = 15; // Second punch damage
    int attack_damage3 = 25; // Third punch (finisher) damage
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
    float attack_cooldown = 2.0f;
    float last_attack_time = 0.0f;

    // Detection ranges
    float detection_range = 300.0f;
    float chase_range = 400.0f;
    float attack_range = 80.0f;
  };

  struct Attack
  {
    int damage = 10;

    Attack() = default;
    explicit Attack(int dmg) : damage(dmg) {}
  };

} // namespace Components

REGISTER_COMPONENT(Components::Player, walk_speed, air_speed, jump_force, hitbox_offset, hitbox_width, hitbox_height, attack_dash_speed1, attack_dash_speed2, attack_dash_speed3, attack_damage1, attack_damage2, attack_damage3)
REGISTER_COMPONENT(Components::LazyMonkey, chase_speed, leap_speed, leap_height, attack_cooldown, last_attack_time, detection_range, chase_range, attack_range)
REGISTER_COMPONENT(Components::Attack, damage)

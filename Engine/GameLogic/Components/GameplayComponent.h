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
    float attack_dash_speed1 = 1000.0f; // First punch forward movement
    float attack_dash_speed2 = 1000.0f; // Second punch forward movement
    float attack_dash_speed3 = 2000.0f; // Third punch (finisher) forward movement
    int attack_damage1 = 10; // First punch damage
    int attack_damage2 = 15; // Second punch damage
    int attack_damage3 = 25; // Third punch (finisher) damage

    // Attack knockbacks
    Math::Vec2D attack_knockback1 = Math::Vec2D(1000.0f, -300.0f);
    Math::Vec2D attack_knockback2 = Math::Vec2D(1000.0f, -300.0f);
    Math::Vec2D attack_knockback3 = Math::Vec2D(2000.0f, -300.0f);
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

} // namespace Components

REGISTER_COMPONENT(Components::Player, walk_speed, air_speed, jump_force, hitbox_offset, hitbox_width, hitbox_height, attack_dash_speed1, attack_dash_speed2, attack_dash_speed3, attack_damage1, attack_damage2, attack_damage3, attack_knockback1, attack_knockback2, attack_knockback3)
REGISTER_COMPONENT(Components::LazyMonkey, chase_speed, leap_speed, leap_height, attack_cooldown, last_attack_time, detection_range, chase_range, attack_range)
REGISTER_COMPONENT(Components::Attack, damage, knockback)

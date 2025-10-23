#pragma once
#include "../Math/Vector2D.h"
#include "./typedef.h"

namespace Components
{
  // Note to MX, feel free to change anything here im just adding this so all the demos work
  struct RigidBody {
    bool detect_collisions = false;
    bool is_kinematic = false; // If true, not affected by physics forces
    bool use_gravity = false;

    f32 mass = 1.0f; // Mass in kg
    Math::Vec2D force{};
    Math::Vec2D acceleration{};
    Math::Vec2D velocity{}; // Velocity in units per second
    // terminal velocity should be a set value instead of a vector just cuz you want to properly scale the velocity instead of capping it at set values
    f32 terminal_velocity{ 300 }; // idk i wack some value here first

    // no use for these yet
    f32 rotation = 0.0f; // In degrees
    f32 rotation_terminal_velocity = 360.0f; // In degrees per second
    f32 angular_velocity = 0.0f; // In degrees per second
  };
}

REFLECT_FIELDS(Components::RigidBody, detect_collisions, linear_velocity, is_kinematic, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity)
#pragma once
#include "../Math/Vector2D.h"

namespace Components
{
  // Note to MX, feel free to change anything here im just adding this so all the demos work
  struct RigidBody {
    bool detect_collisions = true;
    Math::Vec2D linear_velocity = { 0.0f, 0.0f }; // Velocity in units per second
    bool is_kinematic = false; // If true, not affected by physics forces
    float mass = 1.0f; // Mass in kg
    Math::Vec2D linear_terminal_velocity = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    float rotation = 0.0f; // In degrees
    float rotation_terminal_velocity = 360.0f; // In degrees per second
    float angular_velocity = 0.0f; // In degrees per second
  };
}

REFLECT(Components::RigidBody, detect_collisions, linear_velocity, is_kinematic, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity)
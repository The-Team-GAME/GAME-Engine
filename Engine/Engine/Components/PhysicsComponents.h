#pragma once
#include "../Math/Vector2D.h"
#include "./typedef.h"

namespace Components
{
  // https://docs.unity3d.com/ScriptReference/Rigidbody-isKinematic.html
  /* Let the rigidbody take control and detect collisions. */
  // rb.isKinematic = false;
  // rb.detectCollisions = true;

  /* Let animation control the rigidbody and ignore collisions. */
  // rb.isKinematic = true;
  // rb.detectCollisions = false;

  /* Static body that is not affected by forces but will make other bodies react. (eg. static floor) */
  // rb.isKinematic = true;
  // rb.detectCollisions = true;

  // Note to MX, feel free to change anything here im just adding this so all the demos work

  // https://docs.unity3d.com/ScriptReference/Rigidbody.html
  struct RigidBody {
    bool is_kinematic = false; // If true, not affected by physics forces
    bool detect_collisions = true;
    bool use_gravity = true;
    float mass = 1.0f; // Mass in kg
    float rotation = 0.0f; // In degrees
    Math::Vec2D linear_terminal_velocity = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    float rotation_terminal_velocity = 360.0f; // In degrees per second
    Math::Vec2D linear_velocity = { 0.0f, 0.0f }; // Velocity in units per second
    float angular_velocity = 0.0f; // In degrees per second

    RigidBody(bool kinematic = false, bool collisions = true, bool gravity = true, float m = 1.0f, float rot = 0.0f, Math::Vec2D lin_vel = { 0.0f, 0.0f }, float ang_vel = 0.0f)
      : is_kinematic(kinematic), detect_collisions(collisions), use_gravity(gravity), mass(m), rotation(rot), linear_velocity(lin_vel), angular_velocity(ang_vel) {
    }
  };


  // https://docs.unity3d.com/6000.2/Documentation/Manual/class-BoxCollider.html
  struct BoxCollider {
    Math::Vec2D size; // Width and height

    BoxCollider(f32 width = 1.0f, f32 height = 1.0f)
      : size(width, height) {
    }
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
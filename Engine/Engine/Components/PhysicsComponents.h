#pragma once
#include "../Math/Vector2D.h"
#include "../typedef.h"

using namespace Math;

namespace Components
{
  enum Shapes {
    CIRCLE = 0,
    RECT = 1
  };
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
    bool use_gravity = false;

    f32 mass = 1.0f; // Mass in kg
    Vec2D force;
    Vec2D acceleration;
    Vec2D velocity; // Velocity in units per second
    // terminal velocity should be a set value instead of a vector just cuz you want to properly scale the velocity instead of capping it at set values
    f32 terminal_velocity{ 300 }; // idk i wack some value here first

    // no use for these yet
    f32 rotation = 0.0f; // In degrees
    f32 rotation_terminal_velocity = 360.0f; // In degrees per second
    f32 angular_velocity = 0.0f; // In degrees per second

    RigidBody(bool kinematic, bool gravity): 
      is_kinematic(kinematic),
      use_gravity(gravity)
    { /* empty by design */ }
  };


  // https://docs.unity3d.com/6000.2/Documentation/Manual/class-BoxCollider.html
  struct BoxCollider {
    bool detect_collisions = false;
    // measurements for rectangles and circles
    f32 width;
    f32 height;
    f32 radius;

    Vec2D center_pos; // not sure if i wana keep this yet

    BoxCollider(Vec2D center, f32 width, f32 height) : center_pos(center), width(width), height(height)
    { /* empty by design */ }

    BoxCollider(Vec2D center, f32 radius) : center_pos(center), radius(radius)
    { /* empty by design */ }
  };
}

//REGISTER_COMPONENT(Components::RigidBody, detect_collisions, linear_velocity, is_kinematic, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity)
//REFLECT_FIELDS(Components::RigidBody, detect_collisions, linear_velocity, is_kinematic, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity//
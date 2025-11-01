#pragma once
#include "../Math/Vector2D.h"

using namespace Math;

namespace Components
{
  // https://docs.unity3d.com/ScriptReference/Rigidbody-isKinematic.html
  // https://docs.unity3d.com/ScriptReference/Rigidbody.html
  struct RigidBody {
    // Assume that position is the center point of the entity
    Vec2D curr_pos;
    Vec2D next_pos;

    f32 mass = 10.0f; // Mass in kg
    Vec2D force;
    Vec2D acceleration;
    Vec2D velocity; // Velocity in units per second
    // terminal velocity should be a set value instead of a vector just cuz you want to properly scale the velocity instead of capping it at set values
    f32 friction{ 0.995f };
    f32 terminal_velocity{ 300.f }; // idk i wack some value here first

    bool apply_physics = false;  // If true, affected by physics forces
    bool use_gravity = false;   // If true, affected by gravity

    /* 
    no use for these yet*/
    f32 rotation = 0.0f; // In degrees
    f32 rotation_terminal_velocity = 360.0f; // In degrees per second
    f32 angular_velocity = 0.0f; // In degrees per second
    

    // Ctor using vectors
    RigidBody(Vec2D pos = {0,0}, bool kinematic = false, bool gravity = false) :
      curr_pos(pos), apply_physics(kinematic), use_gravity(gravity)
    { 
      if (gravity)
      {
        acceleration.y = 25.f;
      }
    }

    //// Ctor using floats
    //RigidBody(f32 pos_x, f32 pos_y, bool kinematic = false, bool gravity = false) :
    //  curr_pos(Vec2D(pos_x, pos_y)), apply_physics(kinematic), use_gravity(gravity)
    //{ /* empty by design */ }
  };
}

//REGISTER_COMPONENT(Components::RigidBody, detect_collisions, linear_velocity, apply_physics, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity)
//REFLECT_FIELDS(Components::RigidBody, detect_collisions, linear_velocity, apply_physics, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity//
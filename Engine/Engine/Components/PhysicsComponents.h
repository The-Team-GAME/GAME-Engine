//-----------------------------------------------------------------------------------
// file:    PhysicsComponents.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file contains the definition of the data structures that will be
//          needed for physics related logic / calculations
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../Math/Vector2D.h"

using namespace Math;

enum dir {
  LEFT,
  RIGHT
};

namespace Components
{
  // https://docs.unity3d.com/ScriptReference/Rigidbody-isKinematic.html
  // https://docs.unity3d.com/ScriptReference/Rigidbody.html

  struct RigidBody {

    f32 mass = 1.0f; // Mass in kg
    Vec2D force;
    Vec2D acceleration;
    Vec2D gravity_acc;
    Vec2D velocity; // Velocity in units per second
    // terminal velocity should be a set value instead of a vector just cuz you want to properly scale the velocity instead of capping it at set values
    f32 friction{ 0.99f };
    f32 terminal_velocity{ 750.f }; // idk i wack some value here first, might split into the individual axis eventually

    bool is_static = false;  // If true, not affected by physics forces
    bool on_ground = false;
    bool is_jumping = false;

    // Is the rigidbody facing left or right
    // This should be updated in the script
    dir direction; 

    /*
    no use for these yet*/
    f32 rotation = 0.0f; // In degrees
    f32 rotation_terminal_velocity = 360.0f; // In degrees per second
    f32 angular_velocity = 0.0f; // In degrees per second


    // Ctor using vectors
    RigidBody(bool affected_by_forces, bool affected_by_gravity) : is_static(affected_by_forces)
    {
      if (affected_by_gravity) // im setting the gravity acc value so i dont need to keep checking if an object is affected by gravity
      {
        gravity_acc.y = 1400.f;
      }
    }

    // Just to fix reflection temporarily, remove when RigidBody is closer to finalized
    RigidBody(f32 rotation = 0.0f, f32 angular_velocity = 0.0f) : rotation(rotation), angular_velocity(angular_velocity)
    {
    }

    
  };
}

REGISTER_COMPONENT(Components::RigidBody, rotation, angular_velocity)
//REFLECT_FIELDS(Components::RigidBody, resolve_collisions, linear_velocity, is_static, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity//
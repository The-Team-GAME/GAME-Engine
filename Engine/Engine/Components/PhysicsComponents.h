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
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../Math/Vector2D.h"
#include "../ECS/Macros.h"

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
    Vec2D force_view; // This is for debugging purposes
    Vec2D acceleration;
    Vec2D velocity; // Velocity in units per second
    Vec2D gravity{ 0.f, 1000.f };
    // terminal velocity should be a set value instead of a vector just cuz you want to properly scale the velocity instead of capping it at set values
    f32 terminal_velocity{ 1000.f }; // set to max float
    f32 x_terminal_velocity = 500.f;
    f32 y_terminal_velocity = 500.f;

    Vec2D last_surface_dir_vector{ 1.f, 0.f };

    bool use_gravity = false;
    bool is_static = false;  // If true, not affected by physics forces
    bool on_ground = false;
    bool is_jumping = false;

    // Is the rigidbody facing left or right
    // This should be updated in the script
    dir direction; // not sure if its needed

    f32 static_friction_coefficient = 1.f; // not used, maybe ever
    f32 kinetic_coefficient = 0.9995f;

    /*
    no use for these yet*/
    f32 rotation = 0.0f; // In degrees
    f32 rotation_terminal_velocity = 360.0f; // In degrees per second
    f32 angular_velocity = 0.0f; // In degrees per second


    // Ctor using vectors
    RigidBody(bool not_affected_by_forces, bool affected_by_gravity) : is_static(not_affected_by_forces), use_gravity (affected_by_gravity)
    {

    }

    // Default constructor for reflection
    RigidBody() : mass(1.0f), terminal_velocity(750.f), is_static(false), on_ground(false),
      is_jumping(false), direction(RIGHT), rotation(0.0f), rotation_terminal_velocity(360.0f), angular_velocity(0.0f)
    {
    }


  };
}

REGISTER_COMPONENT(Components::RigidBody, mass, force_view, acceleration, velocity, x_terminal_velocity, y_terminal_velocity, use_gravity, gravity, is_static, on_ground, is_jumping)
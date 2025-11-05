#pragma once
#include "../Math/Vector2D.h"
#include "../ECS/Types.h"

using namespace Math;

namespace Components
{
  // https://docs.unity3d.com/ScriptReference/Rigidbody-isKinematic.html
  // https://docs.unity3d.com/ScriptReference/Rigidbody.html
  struct RigidBody {
    // Assume that position is the center point of the entity
    Vec2D curr_pos;
    Vec2D next_pos;

    f32 mass = 1.0f; // Mass in kg
    Vec2D force;
    Vec2D acceleration;
    Vec2D gravity_acc;
    Vec2D velocity; // Velocity in units per second
    // terminal velocity should be a set value instead of a vector just cuz you want to properly scale the velocity instead of capping it at set values
    f32 friction{ 0.99999f };
    f32 terminal_velocity{ 750.f }; // idk i wack some value here first, might split into the individual axis eventually

    bool is_static = false;  // If true, affected by physics forces
    bool on_ground = false;

    // Layer collision filtering (like Unity's Rigidbody includeLayers/excludeLayers)
    // Rule: Entities ALWAYS collide with others in their own layers
    // include_layers: Additional layers this entity should collide with
    // exclude_layers: Layers to NOT collide with (overrides own layers and include_layers)
    // 
    // Example: Player on "player" layer wants to collide with "enemy" but not "player_bullet"
    //   - Player is on layer "player"
    //   - Player.include_layers = LayerSignature{"enemy"}
    //   - Player.exclude_layers = LayerSignature{"player_bullet"}
    //   Result: Player collides with "player" (own layer) and "enemy", but not "player_bullet"
    Engine::ECS::LayerSignature include_layers; // Additional layers to collide with
    Engine::ECS::LayerSignature exclude_layers; // Layers to NOT collide with (overrides include)

    /*
    no use for these yet*/
    f32 rotation = 0.0f; // In degrees
    f32 rotation_terminal_velocity = 360.0f; // In degrees per second
    f32 angular_velocity = 0.0f; // In degrees per second


    // Ctor using vectors
    RigidBody(Vec2D pos, bool kinematic, bool gravity) : curr_pos(pos), is_static(kinematic)
    {
      if (gravity) // im setting the gravity acc value so i dont need to keep checking if an object is affected by gravity
      {
        gravity_acc.y = 200.f;
      }
    }

    // Just to fix reflection temporarily, remove when RigidBody is closer to finalized
    RigidBody(f32 rotation = 0.0f, f32 angular_velocity = 0.0f) : rotation(rotation), angular_velocity(angular_velocity)
    {
    }

    //// Ctor using floats
    //RigidBody(f32 pos_x, f32 pos_y, bool kinematic = false, bool gravity = false) :
    //  curr_pos(Vec2D(pos_x, pos_y)), is_static(kinematic), use_gravity(gravity)
    //{ /* empty by design */ }
  };
}

REGISTER_COMPONENT(Components::RigidBody, rotation, angular_velocity)
//REFLECT_FIELDS(Components::RigidBody, detect_collisions, linear_velocity, is_static, mass, linear_terminal_velocity, rotation, rotation_terminal_velocity, angular_velocity//
//-----------------------------------------------------------------------------------
// file:    PhysicsSystem.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines a system that handles the physics updates for all 
//          entities tht contain rigidbody and transform2d components.
//          Entities will have their rigidbody updated which in turn updates the
//          transform2d component as well.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once


#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Time.h"

namespace Engine {
  namespace Systems
  {
    class Physics2DSystem : public Engine::ECS::System 
    {
    public:
      Physics2DSystem(); // Default dtor that includes all the components needed for the entities called in the physics engine
      
      void update() override;

      void update_entity(Engine::ECS::Entity& entity) override;

      // void knockback(Components::RigidBody& rigid, Vec2D force);
      // Update the linear velocities if said entities here
      void update_kinematics(
        Components::RigidBody& rigid, 
        Components::Transform2D& xform);

      // Called when resolving collisions to zero the rigidbody's velocity that is pushing against the normal
      // of the collision surface while retaining the sliding velocity parallel to the collision surface
      static void zero_normal_velocity(
        Components::RigidBody& rigid, 
        CollisionResult const& collision);

      // Used to apply friction when colliding into another object
      static void apply_surface_friction(
        Components::RigidBody& rigid,
        CollisionResult const& collision,
        f32 static_friction,
        f32 kinetic_friction
      );

      // Used to generate the normal force 
      static f32 estimate_normal_force(
        Components::RigidBody const& rigid,
        CollisionResult const& collision
      );

      // Used to add forces to the rigid body
      static void add_force(Components::RigidBody& rigid, Vec2D force);

      // Used to scale forces acting on the rigid body
      void scale_force(Components::RigidBody& rigid, f32 scale);

      // Used to reset forces acting on the rigid body
      void zero_force(Components::RigidBody& rigid);

      static void walk_left(Components::RigidBody& rigid);

      static void walk_right(Components::RigidBody& rigid);

      static void jump(Components::RigidBody& rigid);


    };
  }
}

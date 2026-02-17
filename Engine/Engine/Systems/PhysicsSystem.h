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
#include "../EngineExport.h"
#include "../Components/Components.h"
#include "../Time.h"

namespace Engine {
  namespace Systems
  {
    class Physics2DSystem : public Engine::ECS::System
    {
    public:
      Physics2DSystem(); // Default Ctor that includes all the components needed for the entities called in the physics engine

      void update() override;

      void update_entity(Engine::ECS::Entity entity) override;

      // void knockback(Components::RigidBody& rigid, Vec2D force);
      // Update the linear velocities if said entities here
      static void update_kinematics(
        Components::RigidBody& rigid);

      static void update_position(
        Components::RigidBody& rigid,
        Components::Transform2D& xform);

      // Called when resolving collisions to zero the rigidbody's velocity that is pushing against the normal
      // of the collision surface while retaining the sliding velocity parallel to the collision surface
      static void zero_normal_velocity(
        Components::RigidBody& rigid,
        CollisionResult const& collision);

      static f32 get_slope_angle_threshold();

      static f32 get_slope_radian_threshold();

      // Used to add forces to the rigid body
      static void add_force(Components::RigidBody& rigid, Vec2D force);


    };
  }
}

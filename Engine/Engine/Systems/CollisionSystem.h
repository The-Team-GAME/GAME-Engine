//-----------------------------------------------------------------------------------
// file:    CollisionSystem.h
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a system that handles entity rotation by updating
//          rotation angles based on rotation speed and delta time in the ECS
//          framework.
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
    class Collision2DSystem : public Engine::ECS::System
    {
    public:
      Collision2DSystem(); // Default dtor that includes all the components needed for the entities called in the physics engine

      // Used to add forces to the rigid body
      void add_force(Components::RigidBody& rigid, Vec2D force);

      // Used to scale forces acting on the rigid body
      void scale_force(Components::RigidBody& rigid, f32 scale);

      // Used to reset forces acting on the rigid body
      void zero_force(Components::RigidBody& rigid);

      // Update the linear velocities if said entities here
      void update_kinematics(Components::RigidBody& rigid);

      void update_transform(Components::RigidBody& rigid);
      // Maybe we check for collision here and then do translate as the last step?
    };
  }
}


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

      // functionally the same as update_AABB but named differently for clarity sake
      void init_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box);

      // update the AABB of the entity after it gets transformed
      void update_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box);

      /*========================/
      / Static Collision Checks /
      /========================*/
      // i think the functions explain themselves kekw
      // read only cuz we only checking for collision and not changing any variables
      // might swap to return an unsigned int after to show which sides are colliding
      bool rect_rect_coll     (Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);
      bool circle_circle_coll (Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);

      // in this scenario the rect should be a wall
      bool rect_circle_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);
    };
  }
}


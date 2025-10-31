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
      // void init_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box);

      // update the AABB of the entity after it gets transformed
      static void update_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box);

      static AABB minkowski_AABB(Components::BoxCollider const& dynamic_obj, Components::BoxCollider const& static_obj)
      {
        AABB minkowski;
        // get the direction vector wrt the static object so that static obj becomes the "origin"
        minkowski.pos = dynamic_obj.aabb.pos - static_obj.aabb.pos;
        f32 half_width = (dynamic_obj.width + static_obj.width) / 2;
        f32 half_height = (dynamic_obj.height + static_obj.height) / 2;
        minkowski.min = Vec2D(minkowski.pos.x - half_width, minkowski.pos.y - half_height);
        minkowski.max = Vec2D(minkowski.pos.x - half_width, minkowski.pos.y - half_height);
        return minkowski;
      }

      static Vec2D penetrationVector(AABB const& minkowski)
      {
        // So... how does this work.
        // First we take the aabb and find the distance to the edges of the static object

        float left = fabsf(minkowski.min.x);
        float right = fabsf(minkowski.max.x);
        float bottom = fabsf(minkowski.min.y);
        float top = fabsf(minkowski.max.y);

        // Then we see which side is shortest
        float min_x = std::min(left, right);
        float min_y = std::min(bottom, top);

        // Preference to the y axis for ground collisions
        if (min_y <= min_x) {
          return Vec2D( 0, (bottom < top) ? minkowski.min.y : minkowski.max.y );
        }
        else {
          return Vec2D( (left < right) ? minkowski.min.x : minkowski.max.x, 0 );
        }
      }
      /*========================/
      / Static Collision Checks /
      /========================*/
      // i think the functions explain themselves kekw
      // read only cuz we only checking for collision and not changing any variables
      // might swap to return an unsigned int after to show which sides are colliding
      static bool rect_rect_coll     (Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);
      bool circle_circle_coll (Components::BoxCollider const& lhs, Components::BoxCollider const& rhs); // wont implement yet

      // in this scenario the rect should be a wall
      bool rect_circle_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);
    };
  }
}


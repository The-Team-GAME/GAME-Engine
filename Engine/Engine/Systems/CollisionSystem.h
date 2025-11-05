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

      void update() override;

      void update_entity(Engine::ECS::Entity& entity) override;

      // update the AABB of the entity after it gets transformed
      static void update_AABB(Components::Transform2D const& xform, Components::BoxCollider& box);

      /*=========================/
      / Minkowski AABB Collision /
      /=========================*/

      static AABB minkowski_AABB(Components::BoxCollider const& dynamic_obj, Components::BoxCollider const& static_obj);

      // This gives the vector for how deep the dynamic object has collided into the static object
      // So u need to take the position of the obj and minus the pen-vec to get the correct location
      static Vec2D penetrationVector(AABB const& minkowski);

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

      /*==============/
      / Quad Tree UwU /
      /==============*/
      // Ok so we are going to use a quad tree to settle our collisions to reduce the number of checks we need to make so that the engine runs smoother

      enum { // enum for the quadrants in the quad tree
        NE, NW, SE, SW
      };
      class QuadTree 
      {
      private:
        AABB boundary; // The "size" of that quadrant to check (initial quadrant will basically be the size of the world, screen for now cuz we just testing)
        std::vector<AABB> entity_vec; // Vector of objects stored at the current node of the quad tree
        QuadTree* children[4]; // NW, NE, SW, SE child nodes

        u32 const max_entities = 10; // The max number of entities that can be in this node before the tree is split again
        u32 const max_depth = 10; // We are capping the number of times the tree can split :>


        bool divided; // To check if the current node has been split
        int depth; // Current depth of tree

      public:

      };
    };
  }
}


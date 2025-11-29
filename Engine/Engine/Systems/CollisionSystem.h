//-----------------------------------------------------------------------------------
// file:    CollisionSystem.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file declares the functions in the system that handles the 
//          collision detection and collision resolution for entities that contain 
//          the components of Transform2D, RigidBody and BoxCollider.
//          As well as the declaration of a QuadTree class that is to be used for 
//          spatial partitioning to ease the number of collision checks that the 
//          engine needs to run.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Time.h"
#include <memory>

// #define USE_OLD_QUAD_TREE;

namespace Engine {
  namespace Systems
  {
    class Collision2DSystem : public Engine::ECS::System
    {

    /*==============/
    / Quad Tree UwU /
    /==============*/
    // Ok so we are going to use a quad tree to settle our collisions to reduce the number of checks we need to make so that the engine runs smoother

    enum { // enum for the quadrants in the quad tree
      NW, NE, SW, SE
    };

    // Old Quad Tree Implementation, ill leave it here for reading
#ifdef USE_OLD_QUAD_TREE
    class QuadTree 
    {
    private:
      AABB bounds; // The "size" of that quadrant to check (initial quadrant will basically be the size of the world, screen for now cuz we just testing)
      std::vector<ECS::Entity> entity_vec; // Vector of objects stored at the current node of the quad tree
      std::array<std::unique_ptr<QuadTree>, 4> child_nodes; // NW, NE, SW, SE child nodes

      u32 const max_entities = 4; // The max number of entities that can be in this node before the tree is split again
      u32 const max_depth = 6; // We are capping the number of times the tree can split :>


      bool divided; // To check if the current node has been split
      u32 depth; // Current depth of tree

      // This function splits the current node into 
      // Private cuz we dont want users to anyhow split the node
      void split();

    public:
      // Ctor
      QuadTree(AABB boundary, u32 depth = 0);
      // The bounds would be the constraint of this quad tree node.
      // The depth would be how deep we are in the tree, with depth being the root / parent node
      // WE GROWING PLANTS BOYS... i might be losing it LOL
        

      // Dtor
      ~QuadTree();

      // Clear all entities from the tree and kills all child nodes
      // To be called at the start of each frame so that the tree can be rebuilt
      void clear();

      // Inserts an entity into the quad tree
      bool insert(ECS::Entity& entity);

      // Insert an entity into a child node
      bool insert_in_child_node(ECS::Entity& entity);

      // Check if an entity is fully contained within the current node's boundary
      bool can_contain(ECS::Entity& entity) const;

      //
      void find_near_entity(AABB const& range, std::vector<ECS::Entity>& nearby);

      // Returns the number of entities in the current node
      u32 entity_count() const;

      // Returns a bool to show if this node has been divided/ has children node already
      bool is_divided() const;

      void draw_tree_node() const;
    };
#endif

    class QuadTreeSAT
    {
    private:
      AABB bounds; // AABB bounds for spatial partitioning (broad-phase)
      std::vector<ECS::Entity> entity_vec; // Entities in this node
      std::array<std::unique_ptr<QuadTreeSAT>, 4> child_nodes; // NW, NE, SW, SE

      u32 const max_entities = 4;
      u32 const max_depth = 6;

      bool divided;
      u32 depth;

      void split();



      // Helper to check if entity's AABB can be contained in this node
      bool can_contain_entity(ECS::Entity& entity) const;

    public:
      QuadTreeSAT(AABB boundary, u32 depth = 0);
      ~QuadTreeSAT();

      void clear();
      bool insert(ECS::Entity& entity);
      bool insert_in_child_node(ECS::Entity& entity);

      // Find nearby entities using AABB broad-phase
      void find_near_entity(const AABB& range, std::vector<ECS::Entity>& nearby);

      u32 entity_count() const;
      bool is_divided() const;
      void draw_tree_node() const;
    };

    // QUad Tree helper functions

    /*=================/
    / End of Quad Tree /
    /=================*/

    private:
      //static std::unique_ptr<QuadTree> spatial_tree; 
      static std::unique_ptr<QuadTreeSAT> spatial_tree_sat;
      AABB world_bounds;
    public:
      Collision2DSystem(); // Default dtor that includes all the components needed for the entities called in the physics engine

      void update() override;

      void update_entity(ECS::Entity& entity) override;

      // update the AABB of the entity after it gets transformed
      // No longer used
      // static void update_AABB(Components::Transform2D const& xform, Components::BoxCollider& box);

      // Helper to get AABB for any collider type (for spatial partitioning)
      // OBB and POLYGON types will use their bounding AABB
      static AABB get_entity_aabb(ECS::Entity& entity);

      static CollisionResult check_collision( 
        Components::BoxCollider const& lhs, 
        Components::BoxCollider const& rhs);

      //static void draw_quad_tree();
      static void draw_quad_tree_sat();

      /*=========================/
      / Minkowski AABB Collision /
      /=========================*/

      static AABB minkowski_AABB(Components::BoxCollider const& dynamic_obj, Components::BoxCollider const& static_obj);

      // This gives the vector for how deep the dynamic object has collided into the static object
      // So u need to take the position of the obj and minus the pen-vec to get the correct location
      static Vec2D penetration_vector(AABB const& minkowski);

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

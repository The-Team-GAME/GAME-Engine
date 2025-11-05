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
#include "CollisionSystem.h"
#include "../Perf.h"
using namespace Engine;
using namespace Systems;
using col2D = Collision2DSystem;

col2D::Collision2DSystem() {
  require_components<Components::Transform2D, Components::RigidBody, Components::BoxCollider>();
}

void col2D::update()
{
  Engine::Perf::start("Collision Update");
  for (ECS::EntityID entity_id : entities) {
    ECS::Entity entity(entity_id);
    update_entity(entity);
  }
  ECS::for_each_entity_pair<Components::RigidBody, Components::BoxCollider>(
    [this](ECS::Entity entity, ECS::Entity other)
    // so [this](ECS::Entity entity, ECS::Entity other) is the callback function
    {
      // Both entities are guaranteed to have RigidBody and BoxCollider
      Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
      Components::RigidBody* other_rigid = other.get_component<Components::RigidBody>();
      Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
      Components::BoxCollider* other_box = other.get_component<Components::BoxCollider>();

      /* START OF BUILDING COLLISION MASKS */

      // Build collision masks for both entities
      ECS::LayerSignature entity_layers = entity.get_layers();
      ECS::LayerSignature entity_collision_mask = entity_layers;
      entity_collision_mask |= rigid->include_layers;
      entity_collision_mask &= ~rigid->exclude_layers;

      ECS::LayerSignature other_layers = other.get_layers();
      ECS::LayerSignature other_collision_mask = other_layers;
      other_collision_mask |= other_rigid->include_layers;
      other_collision_mask &= ~other_rigid->exclude_layers;

      // Check if layers allow collision (bidirectional check)
      // entity must be willing to collide with other's layers AND vice versa
      if ((entity_collision_mask & other_layers).none() || (other_collision_mask & entity_layers).none()) {
        return; // Layer filtering prevents collision
      }

      /* END OF BUILDING COLLISION MASKS */

      // Check AABB collision
      if (!col2D::rect_rect_coll(*box, *other_box)) {
        return; // No collision
      }

      // this gona be the most rabak collision resolution ever
      //std::cout << "TRUE " << box->detect_collisions << " " << other_box->detect_collisions << std::endl;

      // Resolve collision based on detect_collisions flags
      if (box->detect_collisions && !other_box->detect_collisions)
      {
        // other_box is the static one
        Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
        AABB minkowski = col2D::minkowski_AABB(*box, *other_box);
        Vec2D pen_vec = col2D::penetrationVector(minkowski);

        xform->position -= pen_vec;
        if (pen_vec.y == 0) // sideways collision
        {
          rigid->velocity.x = 0.f;
        }
        else // updown collision
        {
          rigid->velocity.y = 0.f;
          if (pen_vec.y > 0)
          {
            rigid->on_ground = true;
          }
        }
        //std::cout << "New pos:  " << rigid->next_pos << std::endl;
      }
      //else if (box->detect_collisions == false && other_box->detect_collisions == true)
       //{
       //  // box is the static one
       //  Components::Transform2D* other_xform = other.get_component<Components::Transform2D>();
       //  AABB minkowski = col2D::minkowski_AABB(*other_box, *box);
       //  Vec2D pen_vec = col2D::penetrationVector(minkowski);
       //  other_rigid->velocity -= pen_vec;
       //  other_rigid->next_pos += pen_vec * Time::get_fixed_dt();
       //  if (pen_vec.y == 0) // sideways collision
       //  {
       //    other_rigid->velocity.x = 0.f;
       //    if (pen_vec.y < 0)
       //    {
       //      // std::cout << "on ground\n";
       //      other_rigid->on_ground = true;
       //    }
       //  }
       //  else // updown collision
       //  {
       //    other_rigid->velocity.y = 0.f;
       //  }
       // update_transform(*other_rigid, *other_xform);
      // }
      // Technically we dont need false && true also cuz that would just be duplicated... i think
      // if both detect_collision is false then no point doing LOL
      // we wont do elastic collisions yet, cuz its not needed so i skip
    }
  );
  Engine::Perf::stop("Collision Update");
}

void col2D::update_entity(Engine::ECS::Entity& entity)
{
  Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
  Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
  Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
  rigid->on_ground = false;
  col2D::update_AABB(*xform, *box);
}

// update the AABB of the entity after it gets transformed
void col2D::update_AABB(Components::Transform2D const& xform, Components::BoxCollider& box)
{
  box.aabb.pos = xform.position;
  return;
}

// collision resolution will be in the physics system cuz it makes more sense there :>
/*==============================/
/ Spatial Partitioning Attempts /
/==============================*/

// Ctor
col2D::QuadTree::QuadTree(AABB boundary, u32 depth) : bounds(boundary), depth(depth), divided(false)
{
  // The bounds would be the constraint of this quad tree node.
  // The depth would be how deep we are in the tree, with depth being the root / parent node
  // WE GROWING PLANTS BOYS... i might be losing it LOL
  entity_vec.reserve(max_entities);
}

// Dtor
col2D::QuadTree::~QuadTree() = default;

// Splitting the current node/ creating its child nodes
//Before : After :
//+-------+       +---+---+
//|       |       | 0 | 1 | (0 = NW, 1 = NE)
//|   1   | --->  +---+---+
//|       |       | 2 | 3 | (2 = SW, 3 = SE)
//+-------+       +---+---+
void col2D::QuadTree::split()
{
  Vec2D mid = bounds.pos;
  f32 hw = bounds.half_width / 2;
  f32 hh = bounds.half_height / 2;

  // remember that the top left is 0,0 :>

  child_nodes[0] = std::make_unique<QuadTree>(
    AABB(Vec2D(mid.x - hw, mid.y - hh), bounds.half_width, bounds.half_height),
    depth + 1);

  child_nodes[1] = std::make_unique<QuadTree>(
    AABB(Vec2D(mid.x + hw, mid.y - hh), bounds.half_width, bounds.half_height),
    depth + 1);

  child_nodes[2] = std::make_unique<QuadTree>(
    AABB(Vec2D(mid.x - hw, mid.y + hh), bounds.half_width, bounds.half_height),
    depth + 1);

  child_nodes[3] = std::make_unique<QuadTree>(
    AABB(Vec2D(mid.x + hw, mid.y + hh), bounds.half_width, bounds.half_height),
    depth + 1);
}
// Clear all entities from the tree and kills all child nodes
// To be called at the start of each frame so that the tree can be rebuilt
void col2D::QuadTree::clear()
{
  // Clearing the objects in the current node
  entity_vec.clear();
  // Recursively clearing all the child nodes
  if (divided)
  {
    for (u32 i{}; i < 4; ++i)
    {
      child_nodes[i]->clear(); // Make the child clear its children
      child_nodes[i].reset();  // HEHE MURDER *stabs*
    }
    divided = false;
  }
}

// Inserts an entity into the quad tree
bool insert(ECS::Entity& entity);

// Check if an entity is fully contained within the current node's boundary
bool contains(ECS::Entity& entity);

// Returns the number of entities in the current node
u32 col2D::QuadTree::entity_count() const
{
  return static_cast<u32>(entity_vec.size());
}

// Returns a bool to show if this node has been divided/ has children node already
bool col2D::QuadTree::is_divided() const
{
  return divided;
}
/*========================/
/ Static Collision Checks /
/========================*/
// i think the functions explain themselves kekw
// read only cuz we only checking for collision and not changing any variables
// might swap to return an unsigned int after to show which sides are colliding
bool col2D::rect_rect_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs)
{
  return lhs.aabb.intersects(rhs.aabb);
}

AABB col2D::minkowski_AABB(Components::BoxCollider const& dynamic_obj, Components::BoxCollider const& static_obj)
{
  AABB minkowski;
  // get the direction vector wrt the static object so that static obj becomes the "origin"
  minkowski.pos = dynamic_obj.aabb.pos - static_obj.aabb.pos;
#if 0
  f32 half_width = (dynamic_obj.width + static_obj.width) / 2;
  f32 half_height = (dynamic_obj.height + static_obj.height) / 2;
  minkowski.min = Vec2D(minkowski.pos.x - half_width, minkowski.pos.y - half_height);
  minkowski.max = Vec2D(minkowski.pos.x + half_width, minkowski.pos.y + half_height);
#endif
  minkowski.half_width = dynamic_obj.aabb.half_width + static_obj.aabb.half_width;
  minkowski.half_height = dynamic_obj.aabb.half_height + static_obj.aabb.half_height;
  return minkowski;
}

// This gives the vector for how deep the dynamic object has collided into the static object
// So u need to take the position of the obj and minus the pen-vec to get the correct location
Vec2D col2D::penetrationVector(AABB const& minkowski)
{
  // So... how does this work.
  // First we take the aabb and find the distance to the edges of the static object
  Vec2D min = minkowski.min();
  Vec2D max = minkowski.max();
  float left = fabsf(min.x);
  float right = fabsf(max.x);
  float bottom = fabsf(min.y);
  float top = fabsf(max.y);

  // Then we see which side is shortest
  float min_x = std::min(left, right);
  float min_y = std::min(bottom, top);

  // Preference to the y axis for ground collisions
  if (min_y <= min_x) {
    return Vec2D(0, (bottom < top) ? min.y : max.y);
  }
  else {
    return Vec2D((left < right) ? min.x : max.x, 0);
  }
}

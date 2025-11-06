//-----------------------------------------------------------------------------------
// file:    CollisionSystem.cpp
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a system that handles the collision detection and
//          collision resolution for entities that contain the components of
//          Transform2D, RigidBody and BoxCollider
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "CollisionSystem.h"
#include "../Perf.h"
#include "../FileIO/Settings.h"
#include "../Graphics/Renderer.h" // for picturing the quad tree
using namespace Engine;
using namespace Systems;
using col2D = Collision2DSystem;
using settings = Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>;

// Need to define cuz we made spatial tree static so we could make a static function to call its draw function in another system
std::unique_ptr<col2D::QuadTree> col2D::spatial_tree = nullptr;

col2D::Collision2DSystem() {
  require_components<Components::Transform2D, Components::RigidBody, Components::BoxCollider>();
  // i know i should use world bounds but ill use screen bounds for now for testing put
  Math::Vec2D win_size = Engine::Graphics::screen_to_virtual({ settings::get().window_width, settings::get().window_height });

  f32 win_width = win_size.x;
  f32 win_height = win_size.y;
  Vec2D mid_point(win_width / 2, win_height / 2);
  world_bounds = AABB(mid_point, win_width, win_height);
  spatial_tree = std::make_unique<QuadTree>(world_bounds, 0);
}

void col2D::update()
{
  Engine::Perf::start("Collision Update");

  // reset the tree from last frame
  spatial_tree->clear();

  // update the entities from last frame and chuck them into the spatial tree
  for (ECS::EntityID entity_id : entities)
  {
    ECS::Entity entity(entity_id);
    update_entity(entity);
    spatial_tree->insert(entity);
  }

  for (ECS::EntityID entity_id : entities)
  {
    ECS::Entity entity(entity_id);
    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
    Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();

    // Build collision mask for this entity
    ECS::LayerSignature entity_layers = entity.get_layers();
    ECS::LayerSignature entity_collision_mask = entity_layers;
    entity_collision_mask |= rigid->include_layers;
    entity_collision_mask &= ~rigid->exclude_layers;

    // Query nearby entities using quadtree
    std::vector<ECS::Entity> nearby;
    // In the future the range for the find near entity could be expanded to the range that entity could travel in that frame, but for now ill double the current entity's aabb
    spatial_tree->find_near_entity(AABB(box->aabb.pos, box->width, box->height), nearby);

    // Check collision only with nearby entities
    for (ECS::Entity& other : nearby)
    {
      // Skip self-collision
      if (entity == other) { continue; }

      Components::RigidBody* other_rigid = other.get_component<Components::RigidBody>();
      Components::BoxCollider* other_box = other.get_component<Components::BoxCollider>();

      // Build collision mask for other entity
      ECS::LayerSignature other_layers = other.get_layers();
      ECS::LayerSignature other_collision_mask = other_layers;
      other_collision_mask |= other_rigid->include_layers;
      other_collision_mask &= ~other_rigid->exclude_layers;

      // Layer filtering (bidirectional check)
      if ((entity_collision_mask & other_layers).none() ||
        (other_collision_mask & entity_layers).none())
      {
        continue;
      }

      // Check if both entities intersect
      if (!box->aabb.intersects(other_box->aabb))
      {
        // skip other_entity if they dont collide
        continue;
      }

      // Collision resolution
      if (box->detect_collisions && !other_box->detect_collisions)
      {
        Components::Transform2D* xform = entity.get_component<Components::Transform2D>();

        AABB minkowski = col2D::minkowski_AABB(*box, *other_box);
        Vec2D pen_vec = col2D::penetrationVector(minkowski);

        xform->position -= pen_vec;
        if (pen_vec.y == 0)
        {  // Sideways collision
          rigid->velocity.x = 0.f;
        }
        else
        {  // Up/down collision
          rigid->velocity.y = 0.f;
          if (pen_vec.y > 0)
          {
            rigid->on_ground = true;
          }
        }
      }
    }
  }

#if 0
  //ECS::for_each_entity_pair<Components::RigidBody, Components::BoxCollider>(
  //  [this](ECS::Entity entity, ECS::Entity other)
  //  // so [this](ECS::Entity entity, ECS::Entity other) is the callback function
  //  {
  //    // Both entities are guaranteed to have RigidBody and BoxCollider
  //    Components::RigidBody* rigid = entity.get_component<Components::RigidBody>();
  //    Components::RigidBody* other_rigid = other.get_component<Components::RigidBody>();
  //    Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
  //    Components::BoxCollider* other_box = other.get_component<Components::BoxCollider>();

  //    /* START OF BUILDING COLLISION MASKS */

  //    // Build collision masks for both entities
  //    ECS::LayerSignature entity_layers = entity.get_layers();
  //    ECS::LayerSignature entity_collision_mask = entity_layers;
  //    entity_collision_mask |= rigid->include_layers;
  //    entity_collision_mask &= ~rigid->exclude_layers;

  //    ECS::LayerSignature other_layers = other.get_layers();
  //    ECS::LayerSignature other_collision_mask = other_layers;
  //    other_collision_mask |= other_rigid->include_layers;
  //    other_collision_mask &= ~other_rigid->exclude_layers;

  //    // Check if layers allow collision (bidirectional check)
  //    // entity must be willing to collide with other's layers AND vice versa
  //    if ((entity_collision_mask & other_layers).none() || (other_collision_mask & entity_layers).none()) {
  //      return; // Layer filtering prevents collision
  //    }

  //    /* END OF BUILDING COLLISION MASKS */

  //    // Check AABB collision
  //    if (!col2D::rect_rect_coll(*box, *other_box)) {
  //      return; // No collision
  //    }

  //    // this gona be the most rabak collision resolution ever
  //    //std::cout << "TRUE " << box->detect_collisions << " " << other_box->detect_collisions << std::endl;

  //    // Resolve collision based on detect_collisions flags
  //    if (box->detect_collisions && !other_box->detect_collisions)
  //    {
  //      // other_box is the static one
  //      Components::Transform2D* xform = entity.get_component<Components::Transform2D>();
  //      AABB minkowski = col2D::minkowski_AABB(*box, *other_box);
  //      Vec2D pen_vec = col2D::penetrationVector(minkowski);

  //      xform->position -= pen_vec;
  //      if (pen_vec.y == 0) // sideways collision
  //      {
  //        rigid->velocity.x = 0.f;
  //      }
  //      else // updown collision
  //      {
  //        rigid->velocity.y = 0.f;
  //        if (pen_vec.y > 0)
  //        {
  //          rigid->on_ground = true;
  //        }
  //      }
  //      //std::cout << "New pos:  " << rigid->next_pos << std::endl;
  //    }
  //    //else if (box->detect_collisions == false && other_box->detect_collisions == true)
  //     //{
  //     //  // box is the static one
  //     //  Components::Transform2D* other_xform = other.get_component<Components::Transform2D>();
  //     //  AABB minkowski = col2D::minkowski_AABB(*other_box, *box);
  //     //  Vec2D pen_vec = col2D::penetrationVector(minkowski);
  //     //  other_rigid->velocity -= pen_vec;
  //     //  other_rigid->next_pos += pen_vec * Time::get_fixed_dt();
  //     //  if (pen_vec.y == 0) // sideways collision
  //     //  {
  //     //    other_rigid->velocity.x = 0.f;
  //     //    if (pen_vec.y < 0)
  //     //    {
  //     //      // std::cout << "on ground\n";
  //     //      other_rigid->on_ground = true;
  //     //    }
  //     //  }
  //     //  else // updown collision
  //     //  {
  //     //    other_rigid->velocity.y = 0.f;
  //     //  }
  //     // update_transform(*other_rigid, *other_xform);
  //    // }
  //    // Technically we dont need false && true also cuz that would just be duplicated... i think
  //    // if both detect_collision is false then no point doing LOL
  //    // we wont do elastic collisions yet, cuz its not needed so i skip
  //  }
  //);
#endif
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

void col2D::draw_quad_tree()
{
  if (spatial_tree)
  {
    spatial_tree->draw_tree_node();
  }
}

#pragma region QuadTree
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

  divided = true;
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
bool col2D::QuadTree::insert(ECS::Entity& entity)
{
  Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
  // Check if the entity intersects with this node of the quad tree
  if (!bounds.intersects(box->aabb))
  {
    // If u reach here then the object shouldnt even be in this node LOL

    return false;
  }

  // Next we check if the node has already been divided before
  // If it hasnt, we check if there is enough space to store it
  if (!divided && entity_vec.size() < max_entities)
  {
    entity_vec.push_back(entity);
    return true;
    // Ok problem solved :>
  }

  // If we have reached this part, it means that the node is not divided and we have hit capacity
  // So now we need to split the node, provided we havent hit our max depth
  if (!divided && depth < max_depth)
  {
    // Now we split kekw
    this->split();
    std::vector<ECS::Entity> remaining_entities;
    for (ECS::Entity& existing_entity : entity_vec)
    {
      // Try to insert the existing entities into the child nodes that have been created
      if (!insert_in_child_node(existing_entity))
      {
        // insert_in_child_node will return true if the existing entity can fit inside the child node
        // else it will return false and we enter this part :>
        remaining_entities.push_back(existing_entity);
      }
    }
    // then now the vector will only be filled with objects that can only fit at this depth as it is too big for the child nodes
    entity_vec = std::move(remaining_entities);
  }

  // So after we split (or not), its time to insert the current entity
  // In this case theres a child node
  if (divided)
  {
    if (insert_in_child_node(entity))
    {
      // insertion successful
      return true;
    }
  }

  // I guess this means we max depth LUL
  entity_vec.push_back(entity);
  return true;
}

// Check if an entity can be fully contained within the current node's boundary
bool col2D::QuadTree::can_contain(ECS::Entity& entity) const
{
  Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
  Vec2D box_min = box->aabb.min();
  Vec2D box_max = box->aabb.max();
  Vec2D bounds_min = bounds.min();
  Vec2D bounds_max = bounds.max();

  return box_min.x >= bounds_min.x &&
    box_min.y >= bounds_min.y &&
    box_max.x <= bounds_max.x &&
    box_max.y <= bounds_max.y;
}

// Insert an entity into a child node
bool col2D::QuadTree::insert_in_child_node(ECS::Entity& entity)
{
  // Try to find a child that fully contains the rectangle
  for (u32 i{}; i < 4; ++i)
  {
    if (child_nodes[i]->can_contain(entity)) {
      // If child can fully contain it, insert recursively
      return child_nodes[i]->insert(entity);
    }
  }

  // No single child can contain it (spans multiple quadrants)
  return false;
}

void col2D::QuadTree::find_near_entity(AABB const& range, std::vector<ECS::Entity>& nearby)
{
  // Exit if search range doesn't intersect this node
  // Cuz remember this function is called recursively so this is the exit criteria
  if (!bounds.intersects(range)) {
    return;
  }

  // Step 2: Check all objects stored at this level
  for (ECS::Entity entity : entity_vec) {
    Components::BoxCollider* box = entity.get_component<Components::BoxCollider>();
    if (range.intersects(box->aabb))
    {
      nearby.push_back(entity);
    }
  }

  // Step 3: Recursively check all children
  if (divided) {
    for (u32 i = 0; i < 4; i++) {
      child_nodes[i]->find_near_entity(range, nearby);
    }
  }
}

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

void col2D::QuadTree::draw_tree_node() const
{
  Vec2D dimensions(bounds.half_width * 2, bounds.half_height * 2);
  f32 rotation = 0.f;
  f32 thickness = 2.f;
  // Color based on entity count - green for entities, gray for empty
  Vec4D color;
  if (entity_vec.empty())
  {
    color = Vec4D(1.f, 1.f, 1.f, 0.7f);  // Gray for empty nodes
  }
  else
  {
    color = Vec4D(0.0f, 1.0f, 0.0f, 0.7f);  // Green =  Entities
  }
  Graphics::draw_rect_outline(bounds.pos, dimensions, rotation, thickness, color);
  if (divided) {
    for (u32 i = 0; i < 4; ++i) {
      child_nodes[i]->draw_tree_node();
    }
  }
}

#pragma endregion Quad Tree Implementation

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

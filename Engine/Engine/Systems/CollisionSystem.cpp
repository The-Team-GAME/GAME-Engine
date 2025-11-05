//-----------------------------------------------------------------------------------
// file:    RotationSystem.h
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

using namespace Engine;
using namespace Systems;
using col2D = Collision2DSystem;

col2D::Collision2DSystem() {
  require_components<Components::RigidBody, Components::BoxCollider>();
}

// update the AABB of the entity after it gets transformed
void col2D::update_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box)
{
  box.aabb.pos = rigid.curr_pos;
  box.aabb.min = box.aabb.pos - Vec2D(box.width / 2, box.height / 2);
  box.aabb.max = box.aabb.pos + Vec2D(box.width / 2, box.height / 2);
  return;
}

// collision resolution will be in the physics system cuz it makes more sense there :>

/*========================/
/ Static Collision Checks /
/========================*/
// i think the functions explain themselves kekw
// read only cuz we only checking for collision and not changing any variables
// might swap to return an unsigned int after to show which sides are colliding
bool col2D::rect_rect_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs)
{
  return  (lhs.aabb.min.x < rhs.aabb.max.x) &&
    (rhs.aabb.min.x < lhs.aabb.max.x) &&
    (lhs.aabb.max.y > rhs.aabb.min.y) &&
    (rhs.aabb.max.y > lhs.aabb.min.y);
}

AABB col2D::minkowski_AABB(Components::BoxCollider const& dynamic_obj, Components::BoxCollider const& static_obj)
{
  AABB minkowski;
  // get the direction vector wrt the static object so that static obj becomes the "origin"
  minkowski.pos = dynamic_obj.aabb.pos - static_obj.aabb.pos;
  f32 half_width = (dynamic_obj.width + static_obj.width) / 2;
  f32 half_height = (dynamic_obj.height + static_obj.height) / 2;
  minkowski.min = Vec2D(minkowski.pos.x - half_width, minkowski.pos.y - half_height);
  minkowski.max = Vec2D(minkowski.pos.x + half_width, minkowski.pos.y + half_height);
  return minkowski;
}

// This gives the vector for how deep the dynamic object has collided into the static object
// So u need to take the position of the obj and minus the pen-vec to get the correct location
Vec2D col2D::penetrationVector(AABB const& minkowski)
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
    return Vec2D(0, (bottom < top) ? minkowski.min.y : minkowski.max.y);
  }
  else {
    return Vec2D((left < right) ? minkowski.min.x : minkowski.max.x, 0);
  }
}

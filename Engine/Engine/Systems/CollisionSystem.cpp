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
using coll2D = Collision2DSystem;

coll2D::Collision2DSystem() {
  require_components<Components::RigidBody, Components::BoxCollider>();
}

// update the AABB of the entity after it gets transformed
void coll2D::update_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box)
{
  box.aabb.pos = rigid.curr_pos;
  box.aabb.min = box.aabb.pos - Vec2D(box.width / 2, box.height / 2);
  box.aabb.max = box.aabb.pos + Vec2D(box.width / 2, box.height / 2);
  return;
}

/*============================/
/ Static Collision Resolution /
/============================*/

/*========================/
/ Static Collision Checks /
/========================*/
// i think the functions explain themselves kekw
// read only cuz we only checking for collision and not changing any variables
// might swap to return an unsigned int after to show which sides are colliding
bool coll2D::rect_rect_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs)
{
  return  (lhs.aabb.min.x < rhs.aabb.max.x) &&
    (rhs.aabb.min.x < lhs.aabb.max.x) &&
    (lhs.aabb.max.y > rhs.aabb.min.y) &&
    (rhs.aabb.max.y > lhs.aabb.min.y);
}

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

// functionally the same as update_AABB but named differently for clarity sake
void coll2D::init_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box)
{
  update_AABB(rigid, box);
  return;
}

// update the AABB of the entity after it gets transformed
void coll2D::update_AABB(Components::RigidBody const& rigid, Components::BoxCollider& box)
{
  Vec2D min = rigid.curr_pos - Vec2D(box.width / 2, box.height / 2);
  Vec2D max = rigid.curr_pos + Vec2D(box.width / 2, box.height / 2);
  box.aabb.min = min;
  box.aabb.max = max;
  return;
}

/*========================/
/ Static Collision Checks /
/========================*/
// i think the functions explain themselves kekw
// read only cuz we only checking for collision and not changing any variables
// might swap to return an unsigned int after to show which sides are colliding
bool coll2D::rect_rect_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs)
{

}
bool coll2D::rect_circle_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);
bool coll2D::circle_circle_coll(Components::BoxCollider const& lhs, Components::BoxCollider const& rhs);
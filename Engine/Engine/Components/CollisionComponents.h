//-----------------------------------------------------------------------------------
// file:    CollisionComponents.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file contains the definition of the data structures that will be
//          needed for collision related logic / calculations
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../Math/Vector2D.h"

struct AABB {
  Vec2D pos;
  // Adding this afterwards
  f32 half_width;
  f32 half_height;

  AABB(Vec2D pos = Vec2D(0, 0), f32 width = 0.f, f32 height = 0.f) : pos(pos), half_width(width / 2), half_height(height / 2)
  {
    // Default Ctor
    // Empty by design
  }

  AABB(f32 width, f32 height) : pos(0, 0), half_width(width / 2), half_height(height / 2)
  {
    // For the old style of AABB so it wont return any problems b4 M2 submissions
    // Empty by design
  }

  Vec2D min() const
  {
    return pos - Vec2D(half_width, half_height);
  }
  Vec2D max() const
  {
    return pos + Vec2D(half_width, half_height);
  }

  bool intersects(AABB const& rhs) const
  {
    // This is the logic
    // (lhs_min.x < rhs_max.x) &&
    // (lhs_max.x > rhs_min.x) &&
    // (lhs_min.y < rhs_max.y) &&
    // (lhs_max.y > rhs_min.y);

    return  (pos.x - half_width < rhs.pos.x + rhs.half_width) &&
      (rhs.pos.x - rhs.half_width < pos.x + half_width) &&
      (pos.y - half_height < rhs.pos.y + rhs.half_height) &&
      (rhs.pos.y - rhs.half_height < pos.y + half_height);
  }
};

enum entity_type
{
  STATIC, // A wall, non responsive
  ATTACK,
  RECEIVE,
  IDLE,
  PARRY
};
namespace Components
{

  // https://docs.unity3d.com/6000.2/Documentation/Manual/class-BoxCollider.html
  struct BoxCollider {

    // If a object has resolve_collisions set to true,that means that other objects that 
    // collide into the current object needs to be snapped out of the current object
    bool resolve_collisions = false;

    f32 width;
    f32 height;
    f32 radius;
    AABB aabb;

    // Rect box collider ctor
    BoxCollider(f32 width, f32 height, bool collide) : width(width), height(height), resolve_collisions(collide)
    {
      aabb.half_width = width / 2;
      aabb.half_height = height / 2;
    }

    // cricle box collider ctor (not used for now)
    BoxCollider(f32 radius) : radius(radius)
    {
      /* empty by design */
    }


  };
}
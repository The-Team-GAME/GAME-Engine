#pragma once

#include "../Math/Vector2D.h"

enum Shapes {
  CIRCLE = 0,
  RECT = 1,
  POLYGON = 2 // prayge we never use this
};

struct AABB {
  Vec2D pos;
  // Adding this afterwards
  f32 half_width;
  f32 half_height;

  AABB(Vec2D pos = Vec2D(0,0), f32 width = 0.f, f32 height = 0.f) : pos(pos), half_width(width / 2), half_height(height / 2)
  {
    // Default Ctor
    // Empty by design
  }

  AABB(f32 width, f32 height) : pos(0,0), half_width(width / 2), half_height(height / 2)
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

    return  (pos.x - half_width < rhs.pos.x + rhs.half_width)   &&
            (rhs.pos.x - rhs.half_width < pos.x + half_width)   &&
            (pos.y - half_height < rhs.pos.y + rhs.half_height) &&
            (rhs.pos.y - rhs.half_height < pos.y + half_height);
  }
};

struct Circle {
  float radius = 1.0f;
};

struct Polygon {
  std::vector<Math::Vec2D> vertices;
};
namespace Components
{

  // https://docs.unity3d.com/6000.2/Documentation/Manual/class-BoxCollider.html
  struct BoxCollider {
    bool detect_collisions = false; // If true, affected by collisions -> will move when colliding with things
    // NOTE: if an object with detect collisions collides with another object without detect collision,
    // The object without detect collisions is unmovable aka a wall for all i care

    // measurements for rectangles and circles

    f32 width;
    f32 height;
    f32 radius;
    AABB aabb;

    // Rect box collider ctor
    BoxCollider(f32 width, f32 height, bool col) : width(width), height(height), detect_collisions(col)
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
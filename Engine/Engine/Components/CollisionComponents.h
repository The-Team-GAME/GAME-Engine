#pragma once

#include "../Math/Vector2D.h"

enum Shapes {
  CIRCLE = 0,
  RECT = 1,
  POLYGON = 2 // prayge we never use this
};

struct AABB {
  Vec2D pos;
  Vec2D min; // Bottom left
  Vec2D max; // Top right
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
    Shapes shape;
    AABB aabb;
    Polygon poly;

    // Rect box collider ctor
    BoxCollider(f32 width, f32 height, bool col) : width(width), height(height), detect_collisions(col)
    { 
      // For now we wont initialise the AABB cuz its gona get updated before the collision check anyways so maybe thats a good thing

      //Vec2D min = center - Vec2D(width / 2, height / 2);
      //Vec2D max = center + Vec2D(width / 2, height / 2);
      //aabb.min = min;
      //aabb.max = max;
    }

    // cricle box collider ctor (not used for now)
    BoxCollider(f32 radius) : radius(radius), shape(CIRCLE)
    { 
      /* empty by design */
    }


  };
}
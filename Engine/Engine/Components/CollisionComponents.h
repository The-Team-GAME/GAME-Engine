#pragma once

#include "../Math/Vector2D.h"

// NOT USED YET, WILL BE IMPLEMENTED IN THE FUTURE
namespace Components
{
  enum Shapes {
    CIRCLE = 0,
    RECT = 1,
    POLYGON = 2 // prayge we never use this
  };

  struct AABB {
    Vec2D min; // Bottom left
    Vec2D max; // Top right
  };

  struct Circle {
    float radius = 1.0f;
  };

  struct Polygon {
    std::vector<Math::Vec2D> vertices;
  };

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
    BoxCollider(f32 width, f32 height) : width(width), height(height), shape(RECT)
    { 
      /* calculate the aabb in collision systems */
    }

    // cricle box collider ctor
    BoxCollider(f32 radius) : radius(radius), shape(CIRCLE)
    { /* empty by design */
    }


  };
}
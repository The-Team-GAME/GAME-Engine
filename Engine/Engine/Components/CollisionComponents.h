#pragma once

#include "../Math/Vector2D.h"


// NOT USED YET, WILL BE IMPLEMENTED IN THE FUTURE
namespace Components
{
  struct AABB2D {
    Math::Vec2D min;
    Math::Vec2D max;
  };

  struct Circle {
    float radius = 1.0f;
  };

  struct Polygon {
    std::vector<Math::Vec2D> vertices;
  };
}
#pragma once

#include "Vector2D.h"
#include "Graphics.h"

template<size_t N>
struct fixed_string {
  constexpr fixed_string(const char(&str)[N]) {
    std::copy_n(str, N, value);
  }
  char value[N];
};


namespace Components {
  template <fixed_string id>
  struct Tag {};

  struct Inactive {};

  struct Position : Math::Vec2D {
    using Math::Vec2D::Vec2D;
  };

  struct Velocity : Math::Vec2D {
    using Math::Vec2D::Vec2D;
  };

  struct Size {
    float width, height;
    Size(float width = 0.0f, float height = 0.0f) : width(width), height(height) {}
  };

  struct Rotation {
    float angle; // in degrees
    Rotation(float angle = 0.0f) : angle(angle) {}
  };

  struct Health {
    int value, max;
    Health(int value = 100, int max = 100) : value(value), max(max) {}

    void heal(int amount) {
      value = std::min(value + amount, max);
    }

    void damage(int amount) {
      value = std::max(value - amount, 0);
    }

    bool isDead() const {
      return value <= 0;
    }
  };

  struct Renderable {};

  struct StaticSprite {
    unsigned int textureId;
    StaticSprite(std::string texturePath)
    {
      textureId = Graphics::loadImage(texturePath.c_str());
    }
  };;

} // namespace Components
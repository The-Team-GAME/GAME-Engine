#pragma once

#include "Vector2D.h"
#include "Vector4D.h"
#include "Graphics.h"
#include <string>

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

  struct ObjectId : std::basic_string<char> {
    using std::basic_string<char>::basic_string;
  };

  // Translate
  struct Position : Math::Vec2D {
    using Math::Vec2D::Vec2D;
  };

  struct Scale : Math::Vec2D {
    using Math::Vec2D::Vec2D;
  };

  struct Rotation {
    float angle; // in degrees
    Rotation(float angle = 0.0f) : angle(angle) {}
  };

  struct RotationSpeed {
    float degreesPerSecond;
    RotationSpeed(float degreesPerSecond = 0.0f) : degreesPerSecond(degreesPerSecond) {}
  };

  struct Velocity : Math::Vec2D {
    using Math::Vec2D::Vec2D;
  };

  struct Size {
    float width, height;
    Size(float width = 0.0f, float height = 0.0f) : width(width), height(height) {}
  };

  struct Color : Math::Vec4D {
    using Math::Vec4D::Vec4D;
  };

  struct Background {};

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


  struct PlayerController {
  
  };
   
  struct Collider {
    float halfWidth, halfHeight;
    Math::Vec2D penetration = { 0.0f, 0.0f }; // penetration from last collision
    bool isTrigger = false;                   // optional: whether this collider is a trigger

    Collider(float width = 1.0f, float height = 1.0f, bool trigger = false)
      : halfWidth(width * 0.5f), halfHeight(height * 0.5f), isTrigger(trigger) {
    }
  };

  struct RigidBody {
    f32 mass = 1.0f;             // How heavy the body is
    f32 inverseMass = 1.0f;      // For faster physics (1/mass, 0 if immovable)
    f32 restitution = 0.2f;      // Bounciness (0 = no bounce, 1 = perfectly elastic)
    f32 friction = 0.5f;         // Surface friction
    Math::Vec2D velocity = { 0.0f, 0.0f };   // Current movement speed
    Math::Vec2D acceleration = { 0.0f, 0.0f }; // For applying forces
    bool isStatic = false;         // If true, object doesn’t move
    bool useGravity = true;        // Whether gravity affects it

    RigidBody(f32 m = 1.0f, bool staticBody = false)
      : mass(m), isStatic(staticBody)
    {
      inverseMass = (isStatic || mass == 0.0f) ? 0.0f : 1.0f / mass;
    }
  };

  struct StaticSprite {
    int textureId;
    StaticSprite(std::string texturePath, Graphics::MinifyingFilter minifyingFilter = Graphics::MinifyingFilter::Linear);
    StaticSprite(const StaticSprite& other);
  };

} // namespace Components
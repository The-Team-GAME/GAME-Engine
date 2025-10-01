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
    f32 angle; // in degrees
    Rotation(f32 angle = 0.0f) : angle(angle) {}
  };

  struct RotationSpeed {
    f32 degrees_per_second;
    RotationSpeed(f32 degrees_per_second = 0.0f) : degrees_per_second(degrees_per_second) {}
  };

  struct Size {
    f32 width, height;
    Size(f32 width = 0.0f, f32 height = 0.0f) : width(width), height(height) {}
  };

  struct Color : Math::Vec4D {
    using Math::Vec4D::Vec4D;
  };

  struct Background {};

  struct Health {
    s32 value, max;
    Health(int value = 100, s32 max = 100) : value(value), max(max) {}

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
    //bool isGrounded = false; // Whether the player is on the ground
    PlayerController(/*bool isgrounded = false*/)
      /* : isGrounded(isgrounded)*/ {
    }
  };

  struct Collider {
    f32 halfWidth, halfHeight;
    Math::Vec2D penetration = { 0.0f, 0.0f }; // penetration from last collision
		Math::Vec2D dynamicNormal = { 0.0f, 0.0f }; // normal of last dynamic collision
    f32 dynamicCollisionTime = 0.0f; // cooldown timer to prevent repeated dynamic collisions
    
    bool isStatic = false;                   // optional: whether this collider is a trigger

    Collider(float width = 1.0f, float height = 1.0f, bool isstatic = false)
      : halfWidth(width * 0.5f), halfHeight(height * 0.5f), isStatic(isstatic) {
    }
  };

  struct Gravity {
    f32 strength = 9.81f; // default gravity
    Gravity(float g = 9.81f) : strength(g) {}
  };

  struct RigidBody {
    f32 mass = 1.0f;                      // How heavy the body is
    f32 inverseMass = 1.0f;               // For faster physics (1/mass, 0 if immovable)
    f32 restitution = 0.0f;               // Bounciness (0 = no bounce, 1 = perfectly elastic)
    f32 friction = 50000.0f;                  // Surface friction coefficient (0 = no friction, 1 = max)
    Math::Vec2D velocity = { 0.0f, 0.0f }; // Current movement speed
    Math::Vec2D acceleration = { 0.0f, 0.0f }; // For applying force
    bool useGravity = true;               // Whether gravity affects it
    bool isGrounded = false;              // Whether the body is on the ground

    RigidBody(f32 m = 1.0f, bool staticBody = false)
      : mass(m)
    {
      if (staticBody || m <= 0.0f) {
        mass = 0.0f;
        inverseMass = 0.0f; // immovable
      }
      else {
        inverseMass = 1.0f / mass;
      }
    }
  };

  struct StaticSprite {
    s32 texture_id;
    StaticSprite(std::string texturePath, Graphics::MinifyingFilter minifyingFilter = Graphics::MinifyingFilter::Linear);
    StaticSprite(const StaticSprite& other);
  };

} // namespace Components
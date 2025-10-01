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
    f32 degreesPerSecond;
    RotationSpeed(f32 degreesPerSecond = 0.0f) : degreesPerSecond(degreesPerSecond) {}
  };

  struct Velocity : Math::Vec2D {
    using Math::Vec2D::Vec2D;
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

  struct StaticSprite {
    s32 textureId;
    StaticSprite(s32 textureId = -1) : textureId(textureId) {}
  };

  struct AnimatedSprite {
    s32 spriteSheetId;
    f32 animationTime;
    bool isPlaying;
    bool loop;

    AnimatedSprite(s32 spriteSheetId = -1, bool autoPlay = true, bool loop = true)
      : spriteSheetId(spriteSheetId), animationTime(0.0f), isPlaying(autoPlay), loop(loop) {}

    void play() {
      isPlaying = true;
    }

    void pause() {
      isPlaying = false;
    }

    void stop() {
      isPlaying = false;
      animationTime = 0.0f;
    }

    void reset() {
      animationTime = 0.0f;
    }
  };

  struct SimplePlayerControl {
    f32 speed;
    SimplePlayerControl(f32 speed = 100.0f) : speed(speed) {}
  };

} // namespace Components
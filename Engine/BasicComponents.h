//-----------------------------------------------------------------------------------
// file:    BasicComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines common components used throughout the game engine.
//          Includes transform components (Position, Scale, Rotation), rendering
//          components (Color, StaticSprite, AnimatedSprite), gameplay components
//          (Health, Velocity), and utility components (Tag, ObjectId).
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Vector2D.h"
#include "Vector4D.h"
#include "Graphics.h"
#include <string>

/* ======================================= //
// Fixed string for compile-time tags     //
// ======================================= */
template<size_t N>
struct fixed_string {
  constexpr fixed_string(const char(&str)[N]) {
    std::copy_n(str, N, value);
  }
  char value[N];
};


namespace Components {
  /* ====================== //
  // Identification tags    //
  // ====================== */
  template <fixed_string id>
  struct Tag {};

  struct ObjectId : std::basic_string<char> {
    using std::basic_string<char>::basic_string;
  };

  /* ====================== //
  // Transform components   //
  // ====================== */
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

  /* ====================== //
  // Physics components     //
  // ====================== */
  struct Velocity : Math::Vec2D {
    using Math::Vec2D::Vec2D;
  };

  struct Size {
    f32 width, height;
    Size(f32 width = 0.0f, f32 height = 0.0f) : width(width), height(height) {}
  };

  /* ====================== //
  // Rendering components   //
  // ====================== */
  struct Color : Math::Vec4D {
    using Math::Vec4D::Vec4D;
  };

  struct Background {}; // Tag for background entities

  /* ====================== //
  // Gameplay components    //
  // ====================== */
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

  struct Renderable {}; // Tag for renderable entities

  /* ====================== //
  // Sprite components      //
  // ====================== */
  struct StaticSprite {
    s32 texture_id;
    StaticSprite(s32 texture_id = -1) : texture_id(texture_id) {}
  };

  struct AnimatedSprite {
    s32 spriteSheetId;
    f32 animationTime;
    bool isPlaying;
    bool loop;

    AnimatedSprite(s32 spriteSheetId = -1, bool autoPlay = true, bool loop = true)
      : spriteSheetId(spriteSheetId), animationTime(0.0f), isPlaying(autoPlay), loop(loop) {
    }

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

    // Check if animation has completed one full cycle
    bool hasCompleted(f32 totalDuration) const {
      return !loop && animationTime >= totalDuration;
    }
  };

  /* ====================== //
  // Input components       //
  // ====================== */
  struct SimplePlayerControl {
    f32 speed;
    SimplePlayerControl(f32 speed = 100.0f) : speed(speed) {}
  };

} // namespace Components
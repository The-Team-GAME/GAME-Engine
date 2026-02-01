//-----------------------------------------------------------------------------------
// file:    SpatialComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for spatial transformations and positioning.
//          Includes Transform2D for position, rotation, and scale; ZIndex for
//          rendering order; and ScreenTransform2D for screen-space coordinates.
//          Provides the foundation for entity positioning and hierarchy.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"
#include "../ECS/Macros.h"
#include <vector>
#include <string>

namespace Components
{
  struct Transform2D
  {
    Math::Vec2D position;
    float rotation = 0.0f; // In degrees
    Math::Vec2D scale = { 1.0f, 1.0f };

    Transform2D() : position(0.0f, 0.0f), rotation(0.0f), scale(1.0f, 1.0f) {}
    Transform2D(const Math::Vec2D& pos, float rot = 0.0f, const Math::Vec2D& scl = Math::Vec2D(1.0f, 1.0f))
      : position(pos), rotation(rot), scale(scl) {
    }
    Transform2D(float x, float y, float rot, float scaleX, float scaleY)
      : position(x, y), rotation(rot), scale(scaleX, scaleY) {
    }
  };

  // ============================================================================
  // Transform Keyframe Animation
  // ============================================================================

  /// A single keyframe in a transform animation with per-property easing
  struct TransformKeyframe
  {
    Math::Vec2D position{ 0.0f, 0.0f };
    float rotation = 0.0f;
    Math::Vec2D scale{ 1.0f, 1.0f };

    float time = 0.0f;  // Normalized time (0.0 to 1.0) when this keyframe occurs

    // Per-property easing functions (names from FunctionRegistry)
    std::string position_easing = "EaseLinearNone";
    std::string rotation_easing = "EaseLinearNone";
    std::string scale_easing = "EaseLinearNone";

    TransformKeyframe() = default;
    TransformKeyframe(const Math::Vec2D& pos, float rot, const Math::Vec2D& scl, float t,
      const std::string& pos_ease = "EaseLinearNone",
      const std::string& rot_ease = "EaseLinearNone",
      const std::string& scl_ease = "EaseLinearNone")
      : position(pos), rotation(rot), scale(scl), time(t),
      position_easing(pos_ease), rotation_easing(rot_ease), scale_easing(scl_ease) {
    }
  };

  /// Component for animating Transform2D over time using keyframes
  struct TransformAnimation
  {
    std::vector<TransformKeyframe> keyframes;

    float duration = 1.0f;        // Total animation duration in seconds
    float elapsed = 0.0f;         // Current elapsed time
    float start_time = -1.0f;     // When the animation started (-1 = not started)

    bool is_playing = true;       // Whether the animation is currently playing
    bool is_looping = false;      // Whether to loop when finished
    bool ping_pong = false;       // Reverse direction on each loop

    // Internal state
    bool is_reversed = false;     // Current direction (for ping_pong)
    bool has_initialized = false; // Whether animation has started

    TransformAnimation() = default;
  };

}

REGISTER_COMPONENT(Components::Transform2D, position, rotation, scale)
REGISTER_COMPONENT(Components::TransformKeyframe, position, rotation, scale, time, position_easing, rotation_easing, scale_easing)
REGISTER_COMPONENT(Components::TransformAnimation, keyframes, duration, elapsed, is_playing, is_looping, ping_pong)

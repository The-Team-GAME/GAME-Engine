//-----------------------------------------------------------------------------------
// file:    AnimatorComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Generic animation system components that can animate any numeric field
//          in any component using reflection. Supports keyframe-based animations
//          with per-track easing functions and runtime pointer caching for
//          optimal performance.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/Macros.h"
#include <vector>
#include <string>

namespace Components
{
  // ============================================================================
  // Generic Animator System
  // ============================================================================

  /// A single keyframe for a float value
  struct FloatKeyframe
  {
    float value = 0.0f;
    float time = 0.0f;      // Normalized time (0.0 to 1.0)
    std::string easing = "EaseLinearNone";

    FloatKeyframe() = default;
    FloatKeyframe(float val, float t, const std::string& ease = "EaseLinearNone")
      : value(val), time(t), easing(ease) {
    }
  };

  /// A track that animates a single field in a component
  struct AnimationTrack
  {
    std::string component_type;   // e.g., "Components::Color"
    std::string field_path;       // e.g., "r" or "position.x"
    std::vector<FloatKeyframe> keyframes;

    AnimationTrack() = default;
    AnimationTrack(const std::string& comp, const std::string& field)
      : component_type(comp), field_path(field) {
    }
  };

  /// The main Animator component - animates multiple tracks on an entity
  struct Animator
  {
    std::vector<AnimationTrack> tracks;

    float duration = 1.0f;        // Total animation duration in seconds
    float elapsed = 0.0f;         // Current elapsed time
    float start_time = -1.0f;     // When the animation started

    bool is_playing = true;       // Whether the animation is currently playing
    bool is_looping = true;      // Whether to loop when finished
    bool ping_pong = false;       // Reverse direction on each loop

    // Internal state
    bool is_reversed = false;     // Current direction (for ping_pong)
    bool has_initialized = false; // Whether animation has started
    bool force_update = false;    // Force apply animation at current elapsed (for editor scrubbing)

    Animator() = default;
  };

}

REGISTER_COMPONENT(Components::FloatKeyframe, value, time, easing)
REGISTER_COMPONENT(Components::AnimationTrack, component_type, field_path, keyframes)
REGISTER_COMPONENT(Components::Animator, tracks, duration, elapsed, is_playing, is_looping, ping_pong)

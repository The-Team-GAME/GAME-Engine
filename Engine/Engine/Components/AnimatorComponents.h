//-----------------------------------------------------------------------------------
// file:    AnimatorComponents.h
// project: Monkey Stole My GPA
//
// brief:   Property animation data types for the StateMachine Animator system.
//          Provides keyframe-based animation tracks that can animate any numeric
//          component field (float, int, Vec2D, Color) over time. Integrates with
//          the StateMachine component so each state can carry a PropertyAnimationClip.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Reflection/ReflectionLibrary/Macros.h"
#include "../Reflection/ReflectionLibrary/GenericValue.h"
#include "../Easing.h"
#include "../Math/Vector2D.h"
#include <vector>
#include <string>
#include <cmath>

namespace Components
{

  // ============================================================================
  // Easing Type Enum
  // ============================================================================

  enum class EasingType {
    Linear,
    SineIn,
    SineOut,
    SineInOut,
    CubicIn,
    CubicOut,
    CubicInOut,
    QuadIn,
    QuadOut,
    QuadInOut,
    ExpoIn,
    ExpoOut,
    ExpoInOut,
    CircIn,
    CircOut,
    CircInOut,
    BackIn,
    BackOut,
    BackInOut,
    BounceIn,
    BounceOut,
    BounceInOut,
    ElasticIn,
    ElasticOut,
    ElasticInOut
  };

  /// Evaluate a normalized easing function (t in [0, 1] → result in [0, 1])
  inline float evaluate_easing(EasingType type, float t) {
    using namespace Engine::Easing;
    switch (type) {
    case EasingType::Linear:       return EaseLinearNone(t, 0.0f, 1.0f, 1.0f);
    case EasingType::SineIn:       return EaseSineIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::SineOut:      return EaseSineOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::SineInOut:    return EaseSineInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::CubicIn:      return EaseCubicIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::CubicOut:     return EaseCubicOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::CubicInOut:   return EaseCubicInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::QuadIn:       return EaseQuadIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::QuadOut:      return EaseQuadOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::QuadInOut:    return EaseQuadInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::ExpoIn:       return EaseExpoIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::ExpoOut:      return EaseExpoOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::ExpoInOut:    return EaseExpoInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::CircIn:       return EaseCircIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::CircOut:      return EaseCircOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::CircInOut:    return EaseCircInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::BackIn:       return EaseBackIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::BackOut:      return EaseBackOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::BackInOut:    return EaseBackInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::BounceIn:     return EaseBounceIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::BounceOut:    return EaseBounceOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::BounceInOut:  return EaseBounceInOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::ElasticIn:    return EaseElasticIn(t, 0.0f, 1.0f, 1.0f);
    case EasingType::ElasticOut:   return EaseElasticOut(t, 0.0f, 1.0f, 1.0f);
    case EasingType::ElasticInOut: return EaseElasticInOut(t, 0.0f, 1.0f, 1.0f);
    default:                       return t;
    }
  }

  /// Get display name for an easing type
  inline const char* easing_type_name(EasingType type) {
    switch (type) {
    case EasingType::Linear:       return "Linear";
    case EasingType::SineIn:       return "Sine In";
    case EasingType::SineOut:      return "Sine Out";
    case EasingType::SineInOut:    return "Sine InOut";
    case EasingType::CubicIn:      return "Cubic In";
    case EasingType::CubicOut:     return "Cubic Out";
    case EasingType::CubicInOut:   return "Cubic InOut";
    case EasingType::QuadIn:       return "Quad In";
    case EasingType::QuadOut:      return "Quad Out";
    case EasingType::QuadInOut:    return "Quad InOut";
    case EasingType::ExpoIn:       return "Expo In";
    case EasingType::ExpoOut:      return "Expo Out";
    case EasingType::ExpoInOut:    return "Expo InOut";
    case EasingType::CircIn:       return "Circ In";
    case EasingType::CircOut:      return "Circ Out";
    case EasingType::CircInOut:    return "Circ InOut";
    case EasingType::BackIn:       return "Back In";
    case EasingType::BackOut:      return "Back Out";
    case EasingType::BackInOut:    return "Back InOut";
    case EasingType::BounceIn:     return "Bounce In";
    case EasingType::BounceOut:    return "Bounce Out";
    case EasingType::BounceInOut:  return "Bounce InOut";
    case EasingType::ElasticIn:    return "Elastic In";
    case EasingType::ElasticOut:   return "Elastic Out";
    case EasingType::ElasticInOut: return "Elastic InOut";
    default:                       return "Unknown";
    }
  }

  // ============================================================================
  // Keyframe
  // ============================================================================

  struct Keyframe {
    float time = 0.0f;  // Time in seconds
    double value = 0.0;  // Numeric value at this keyframe

    Keyframe() = default;
    Keyframe(float t, double v) : time(t), value(v) {}
  };

  // ============================================================================
  // Animation Track
  // ============================================================================

  struct AnimationTrack {
    std::string target_component;   // e.g. "Components::Transform2D"
    std::string target_field_path;  // Dot-separated field path e.g. "position.x", "rotation", "r"
    std::vector<Keyframe> keyframes;
    EasingType easing = EasingType::Linear;

    AnimationTrack() = default;

    /// Evaluate the track at a given time, returning the interpolated value
    double evaluate(float time) const {
      if (keyframes.empty()) return 0.0;
      if (keyframes.size() == 1) return keyframes[0].value;

      // Clamp to keyframe range
      if (time <= keyframes.front().time) return keyframes.front().value;
      if (time >= keyframes.back().time) return keyframes.back().value;

      // Find surrounding keyframes (keyframes assumed sorted by time)
      size_t i = 0;
      for (; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time)
          break;
      }

      const Keyframe& kf_a = keyframes[i];
      const Keyframe& kf_b = keyframes[i + 1];

      float segment_duration = kf_b.time - kf_a.time;
      if (segment_duration <= 0.0f) return kf_a.value;

      float local_t = (time - kf_a.time) / segment_duration;
      float eased_t = evaluate_easing(easing, local_t);

      return kf_a.value + (kf_b.value - kf_a.value) * static_cast<double>(eased_t);
    }

    /// Sort keyframes by time
    void sort_keyframes() {
      std::sort(keyframes.begin(), keyframes.end(),
        [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    }
  };

  // ============================================================================
  // Property Animation Clip
  // ============================================================================

  struct PropertyAnimationClip {
    float duration = 1.0f;
    bool is_looping = false;
    std::vector<AnimationTrack> tracks;

    PropertyAnimationClip() = default;
  };

} // namespace Components

// ============================================================================
// Reflection Registration
// ============================================================================

REFLECT_ENUM(Components::EasingType,
  Linear,
  SineIn, SineOut, SineInOut,
  CubicIn, CubicOut, CubicInOut,
  QuadIn, QuadOut, QuadInOut,
  ExpoIn, ExpoOut, ExpoInOut,
  CircIn, CircOut, CircInOut,
  BackIn, BackOut, BackInOut,
  BounceIn, BounceOut, BounceInOut,
  ElasticIn, ElasticOut, ElasticInOut
)

REFLECT_FIELDS(Components::Keyframe, time, value)
REFLECT_FIELDS(Components::AnimationTrack, target_component, target_field_path, keyframes, easing)
REFLECT_FIELDS(Components::PropertyAnimationClip, duration, is_looping, tracks)

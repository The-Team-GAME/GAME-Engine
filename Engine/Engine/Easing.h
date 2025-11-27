//-----------------------------------------------------------------------------------
// file:    Easing.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Collection of easing functions for smooth animations and transitions.
//          Based on Robert Penner's easing equations. Each function takes:
//          t = current time, b = beginning value, c = change in value, d = duration
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "Math/Math.h"
#include "FunctionRegistry.h"


namespace Engine {
  namespace Easing {

    // ============================================================================
    // Linear Easing
    // ============================================================================

    static inline float EaseLinearNone(float t, float b, float c, float d) { return (c * t / d + b); }      // Ease: Linear
    static inline float EaseLinearIn(float t, float b, float c, float d) { return (c * t / d + b); }        // Ease: Linear In
    static inline float EaseLinearOut(float t, float b, float c, float d) { return (c * t / d + b); }       // Ease: Linear Out
    static inline float EaseLinearInOut(float t, float b, float c, float d) { return (c * t / d + b); }     // Ease: Linear In Out

    // ============================================================================
    // Sine Easing
    // ============================================================================

    static inline float EaseSineIn(float t, float b, float c, float d) { return (-c * cosf(t / d * (Math::PI / 2.0f)) + c + b); }
    static inline float EaseSineOut(float t, float b, float c, float d) { return (c * sinf(t / d * (Math::PI / 2.0f)) + b); }
    static inline float EaseSineInOut(float t, float b, float c, float d) { return (-c / 2.0f * (cosf(Math::PI * t / d) - 1.0f) + b); }

    // ============================================================================
    // Circular Easing
    // ============================================================================

    static inline float EaseCircIn(float t, float b, float c, float d) { t /= d; return (-c * (sqrtf(1.0f - t * t) - 1.0f) + b); }
    static inline float EaseCircOut(float t, float b, float c, float d) { t = t / d - 1.0f; return (c * sqrtf(1.0f - t * t) + b); }
    static inline float EaseCircInOut(float t, float b, float c, float d) {
      if ((t /= d / 2.0f) < 1.0f) return (-c / 2.0f * (sqrtf(1.0f - t * t) - 1.0f) + b);
      t -= 2.0f; return (c / 2.0f * (sqrtf(1.0f - t * t) + 1.0f) + b);
    }

    // ============================================================================
    // Cubic Easing
    // ============================================================================

    static inline float EaseCubicIn(float t, float b, float c, float d) { t /= d; return (c * t * t * t + b); }
    static inline float EaseCubicOut(float t, float b, float c, float d) { t = t / d - 1.0f; return (c * (t * t * t + 1.0f) + b); }
    static inline float EaseCubicInOut(float t, float b, float c, float d) {
      if ((t /= d / 2.0f) < 1.0f) return (c / 2.0f * t * t * t + b);
      t -= 2.0f; return (c / 2.0f * (t * t * t + 2.0f) + b);
    }

    // ============================================================================
    // Quadratic Easing
    // ============================================================================

    static inline float EaseQuadIn(float t, float b, float c, float d) { t /= d; return (c * t * t + b); }
    static inline float EaseQuadOut(float t, float b, float c, float d) { t /= d; return (-c * t * (t - 2.0f) + b); }
    static inline float EaseQuadInOut(float t, float b, float c, float d) {
      if ((t /= d / 2.0f) < 1.0f) return (c / 2.0f * t * t + b);
      --t; return (-c / 2.0f * (t * (t - 2.0f) - 1.0f) + b);
    }

    // ============================================================================
    // Exponential Easing
    // ============================================================================

    static inline float EaseExpoIn(float t, float b, float c, float d) { return (t == 0.0f) ? b : (c * powf(2.0f, 10.0f * (t / d - 1.0f)) + b); }
    static inline float EaseExpoOut(float t, float b, float c, float d) { return (t == d) ? (b + c) : (c * (-powf(2.0f, -10.0f * t / d) + 1.0f) + b); }
    static inline float EaseExpoInOut(float t, float b, float c, float d) {
      if (t == 0.0f) return b;
      if (t == d) return (b + c);
      if ((t /= d / 2.0f) < 1.0f) return (c / 2.0f * powf(2.0f, 10.0f * (t - 1.0f)) + b);
      return (c / 2.0f * (-powf(2.0f, -10.0f * --t) + 2.0f) + b);
    }

    // ============================================================================
    // Back Easing
    // ============================================================================

    static inline float EaseBackIn(float t, float b, float c, float d) {
      float s = 1.70158f;
      float postFix = t /= d;
      return (c * (postFix)*t * ((s + 1.0f) * t - s) + b);
    }

    static inline float EaseBackOut(float t, float b, float c, float d) {
      float s = 1.70158f;
      t = t / d - 1.0f;
      return (c * (t * t * ((s + 1.0f) * t + s) + 1.0f) + b);
    }

    static inline float EaseBackInOut(float t, float b, float c, float d) {
      float s = 1.70158f;
      if ((t /= d / 2.0f) < 1.0f) {
        s *= 1.525f;
        return (c / 2.0f * (t * t * ((s + 1.0f) * t - s)) + b);
      }
      float postFix = t -= 2.0f;
      s *= 1.525f;
      return (c / 2.0f * ((postFix)*t * ((s + 1.0f) * t + s) + 2.0f) + b);
    }

    // ============================================================================
    // Bounce Easing
    // ============================================================================

    static inline float EaseBounceOut(float t, float b, float c, float d) {
      if ((t /= d) < (1.0f / 2.75f)) {
        return (c * (7.5625f * t * t) + b);
      }
      else if (t < (2.0f / 2.75f)) {
        float postFix = t -= (1.5f / 2.75f);
        return (c * (7.5625f * (postFix)*t + 0.75f) + b);
      }
      else if (t < (2.5f / 2.75f)) {
        float postFix = t -= (2.25f / 2.75f);
        return (c * (7.5625f * (postFix)*t + 0.9375f) + b);
      }
      else {
        float postFix = t -= (2.625f / 2.75f);
        return (c * (7.5625f * (postFix)*t + 0.984375f) + b);
      }
    }

    static inline float EaseBounceIn(float t, float b, float c, float d) { return (c - EaseBounceOut(d - t, 0.0f, c, d) + b); }
    static inline float EaseBounceInOut(float t, float b, float c, float d) {
      if (t < d / 2.0f) return (EaseBounceIn(t * 2.0f, 0.0f, c, d) * 0.5f + b);
      else return (EaseBounceOut(t * 2.0f - d, 0.0f, c, d) * 0.5f + c * 0.5f + b);
    }

    // ============================================================================
    // Elastic Easing
    // ============================================================================

    static inline float EaseElasticIn(float t, float b, float c, float d) {
      if (t == 0.0f) return b;
      if ((t /= d) == 1.0f) return (b + c);
      float p = d * 0.3f;
      float a = c;
      float s = p / 4.0f;
      float postFix = a * powf(2.0f, 10.0f * (t -= 1.0f));
      return (-(postFix * sinf((t * d - s) * (2.0f * 3.14159265f) / p)) + b);
    }

    static inline float EaseElasticOut(float t, float b, float c, float d) {
      if (t == 0.0f) return b;
      if ((t /= d) == 1.0f) return (b + c);
      float p = d * 0.3f;
      float a = c;
      float s = p / 4.0f;
      return (a * powf(2.0f, -10.0f * t) * sinf((t * d - s) * (2.0f * 3.14159265f) / p) + c + b);
    }

    static inline float EaseElasticInOut(float t, float b, float c, float d) {
      if (t == 0.0f) return b;
      if ((t /= d / 2.0f) == 2.0f) return (b + c);
      float p = d * (0.3f * 1.5f);
      float a = c;
      float s = p / 4.0f;
      if (t < 1.0f) {
        float postFix = a * powf(2.0f, 10.0f * (t -= 1.0f));
        return -0.5f * (postFix * sinf((t * d - s) * (2.0f * 3.14159265f) / p)) + b;
      }
      float postFix = a * powf(2.0f, -10.0f * (t -= 1.0f));
      return (postFix * sinf((t * d - s) * (2.0f * 3.14159265f) / p) * 0.5f + c + b);
    }

    // ============================================================================
    // Registration Helper
    // ============================================================================

    inline void register_all_easing_functions() {
      // Full signature versions (t, b, c, d) -> result
      using EasingFunc = f32(f32, f32, f32, f32);

      // Linear
      FunctionRegistry<EasingFunc>::register_function("EaseLinearNone", EaseLinearNone);
      FunctionRegistry<EasingFunc>::register_function("EaseLinearIn", EaseLinearIn);
      FunctionRegistry<EasingFunc>::register_function("EaseLinearOut", EaseLinearOut);
      FunctionRegistry<EasingFunc>::register_function("EaseLinearInOut", EaseLinearInOut);

      // Sine
      FunctionRegistry<EasingFunc>::register_function("EaseSineIn", EaseSineIn);
      FunctionRegistry<EasingFunc>::register_function("EaseSineOut", EaseSineOut);
      FunctionRegistry<EasingFunc>::register_function("EaseSineInOut", EaseSineInOut);

      // Circular
      FunctionRegistry<EasingFunc>::register_function("EaseCircIn", EaseCircIn);
      FunctionRegistry<EasingFunc>::register_function("EaseCircOut", EaseCircOut);
      FunctionRegistry<EasingFunc>::register_function("EaseCircInOut", EaseCircInOut);

      // Cubic
      FunctionRegistry<EasingFunc>::register_function("EaseCubicIn", EaseCubicIn);
      FunctionRegistry<EasingFunc>::register_function("EaseCubicOut", EaseCubicOut);
      FunctionRegistry<EasingFunc>::register_function("EaseCubicInOut", EaseCubicInOut);

      // Quadratic
      FunctionRegistry<EasingFunc>::register_function("EaseQuadIn", EaseQuadIn);
      FunctionRegistry<EasingFunc>::register_function("EaseQuadOut", EaseQuadOut);
      FunctionRegistry<EasingFunc>::register_function("EaseQuadInOut", EaseQuadInOut);

      // Exponential
      FunctionRegistry<EasingFunc>::register_function("EaseExpoIn", EaseExpoIn);
      FunctionRegistry<EasingFunc>::register_function("EaseExpoOut", EaseExpoOut);
      FunctionRegistry<EasingFunc>::register_function("EaseExpoInOut", EaseExpoInOut);

      // Back
      FunctionRegistry<EasingFunc>::register_function("EaseBackIn", EaseBackIn);
      FunctionRegistry<EasingFunc>::register_function("EaseBackOut", EaseBackOut);
      FunctionRegistry<EasingFunc>::register_function("EaseBackInOut", EaseBackInOut);

      // Bounce
      FunctionRegistry<EasingFunc>::register_function("EaseBounceIn", EaseBounceIn);
      FunctionRegistry<EasingFunc>::register_function("EaseBounceOut", EaseBounceOut);
      FunctionRegistry<EasingFunc>::register_function("EaseBounceInOut", EaseBounceInOut);

      // Elastic
      FunctionRegistry<EasingFunc>::register_function("EaseElasticIn", EaseElasticIn);
      FunctionRegistry<EasingFunc>::register_function("EaseElasticOut", EaseElasticOut);
      FunctionRegistry<EasingFunc>::register_function("EaseElasticInOut", EaseElasticInOut);
    }

  } // namespace Easing
} // namespace Engine

//-----------------------------------------------------------------------------------
// file:    EffectComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines component structs for visual effects, specifically Trail and FadeOut, used for rendering trails and fading entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "../ECS/Macros.h"
#include "../Time.h"

namespace Components {
  struct Trail {
    f32 interval = 0.1f;
    f32 lifetime = 0.5f;
    f32 last_spawn_time = 0.0f;
    f32 start_alpha = 0.5f;
    f32 end_alpha = 0.0f;

    Trail() = default;
    Trail(f32 interval, f32 lifetime, f32 start_alpha = 0.5f, f32 end_alpha = 0.0f)
      : interval(interval), lifetime(lifetime), start_alpha(start_alpha), end_alpha(end_alpha) {
    }
  };

  struct FadeOut {
    f32 duration = 1.0f;
    f32 start_time = 0.0f;
    f32 start_alpha = 1.0f;
    f32 end_alpha = 0.0f;

    FadeOut() = default;
    FadeOut(f32 duration, f32 start_alpha, f32 end_alpha)
      : duration(duration), start_time(0.0f), start_alpha(start_alpha), end_alpha(end_alpha) {
      // Note: start_time should be set manually after construction to ensure proper timing
    }
  };
}

REGISTER_COMPONENT(Components::Trail, interval, lifetime, start_alpha, end_alpha)
REGISTER_COMPONENT(Components::FadeOut, duration, start_time, start_alpha, end_alpha)


//-----------------------------------------------------------------------------------
// file:    HeartrateComponent.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the Heartrate component, which tracks the player's heart rate, manages heart rate states (Low to Dead), and defines visual effects (tint, distortion) associated with each state.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../../Engine/MonkeyEngine.h"

namespace Components {
  struct Heartrate {
    float low = 80.0f;
    float normal = 100.0f;
    float high = 120.0f;
    float very_high = 140.0f;
    float crash_out = 160.0f;
    float dead = 180.0f;

    float heartrate = low;
    float decay_rate = 1.0f; // bpm per second

    // greener the higher the heartrate
    Components::Color low_world_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
    Components::Color normal_world_tint = { 0.95f, 1.0f, 0.95f, 1.0f };
    Components::Color high_world_tint = { 0.9f, 1.0f, 0.9f, 1.0f };
    Components::Color very_high_world_tint = { 0.85f, 1.0f, 0.85f, 1.0f };
    Components::Color crash_out_world_tint = { 0.7f, 1.0f, 0.7f, 1.0f };
    Components::Color dead_world_tint = { 0.7f, 1.0f, 0.7f, 1.0f };

    // Components::Color low_world_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
    // Components::Color normal_world_tint = { 1.0f, 0.8f, 1.0f, 1.0f };
    // Components::Color high_world_tint = { 1.0f, 0.7f, 1.0f, 1.0f };
    // Components::Color very_high_world_tint = { 1.0f, 0.5f, 1.0f, 1.0f };
    // Components::Color crash_out_world_tint = { 1.0f, 0.2f, 1.0f, 1.0f };
    // Components::Color dead_world_tint = { 1.0f, 0.0f, 1.0f, 1.0f };

    // more distortion the higher the heartrate
    float low_distortion_intensity = 0.0f;
    float normal_distortion_intensity = 0.1f;
    float high_distortion_intensity = 0.3f;
    float very_high_distortion_intensity = 0.5f;
    float crash_out_distortion_intensity = 0.8f;
    float dead_distortion_intensity = 1.0f;

    float low_distortion_speed = 0.0f;
    float normal_distortion_speed = 0.1f;
    float high_distortion_speed = 0.2f;
    float very_high_distortion_speed = 0.4f;
    float crash_out_distortion_speed = 0.6f;
    float dead_distortion_speed = 0.8f;

    float low_mesh_distortion_intensity = 0.0f;
    float normal_mesh_distortion_intensity = 0.1f;
    float high_mesh_distortion_intensity = 0.2f;
    float very_high_mesh_distortion_intensity = 0.3f;
    float crash_out_mesh_distortion_intensity = 0.4f;
    float dead_mesh_distortion_intensity = 0.5f;

    float low_trail_lifetime = 0.0f;
    float normal_trail_lifetime = 0.2f;
    float high_trail_lifetime = 0.4f;
    float very_high_trail_lifetime = 0.8f;
    float crash_out_trail_lifetime = 1.0f;
    float dead_trail_lifetime = 1.0f;

    enum class HeartrateState {
      Low,
      Normal,
      High,
      VeryHigh,
      CrashOut,
      Dead
    };

    void increase(float amount) {
      heartrate += amount;
    }

    void decrease(float amount) {
      heartrate -= amount;
      if (heartrate < low) heartrate = low;
    }

    float min_brightness = 0.5f;
    float max_brightness = 1.0f;

    HeartrateState get_state() {
      if (heartrate > dead) return HeartrateState::Dead;
      if (heartrate > crash_out) return HeartrateState::CrashOut;
      if (heartrate > very_high) return HeartrateState::VeryHigh;
      if (heartrate > high) return HeartrateState::High;
      if (heartrate > normal) return HeartrateState::Normal;
      if (heartrate > low) return HeartrateState::Low;
    }
  };
}

REGISTER_COMPONENT(Components::Heartrate, low, normal, high, very_high, crash_out, heartrate, decay_rate, min_brightness, max_brightness)
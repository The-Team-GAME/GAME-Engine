#pragma once
 

namespace Components {
  struct Heartrate {
    float low = 80.0f;
    float normal = 100.0f;
    float high = 120.0f;
    float very_high = 140.0f;
    float crash_out = 160.0f;

    float heartrate = low;
    
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
 
    HeartrateState get_state() {
      if (heartrate < low) return HeartrateState::Low;
      if (heartrate < normal) return HeartrateState::Normal;
      if (heartrate < high) return HeartrateState::High;
      if (heartrate < very_high) return HeartrateState::VeryHigh;
      if (heartrate < crash_out) return HeartrateState::CrashOut;
      return HeartrateState::Dead;
    }
  };
}
#include "Time.h"
#include <chrono>

namespace chrono = std::chrono;

namespace { //consider making it microseconds
  chrono::milliseconds curr_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());

  f64 delta_time{}; // in seconds
  s32 frame_rate{ 60 };
  f64 accumulator = 0.0;
}

namespace Engine {
  namespace Time {
    void begin(void)
    {
      chrono::milliseconds new_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());
      delta_time = static_cast<f64>((new_time - curr_time).count()) / 1000.0;
      curr_time = new_time;
      accumulator += get_internal_dt();
    }

    // This calculates the fixed dt based on the set framerate
    f64 get_fixed_dt(void)
    {
      return 1.0 / frame_rate;
    }

    // Returns the delta time per game loop
    // Only to be used to accumulator in the loop
    f32 get_internal_dt(void)
    {
      return static_cast<f32>(delta_time);
    }

    bool should_update(void)
    {
      return accumulator >= get_fixed_dt();
    }

    void end(void)
    {
      accumulator -= get_fixed_dt();
    }
  }
};
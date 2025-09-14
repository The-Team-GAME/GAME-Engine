#include "Time.h"
#include <chrono>


namespace {
  using clock = std::chrono::high_resolution_clock;
  using time_point = std::chrono::time_point<clock>;
  using duration = std::chrono::duration<f64>;

  constexpr f64 TARGET_FPS = 10.0;
  constexpr u32 MAX_UPDATES_PER_FRAME = 10; // loop cap to avoid spiral of death

  time_point last_time = clock::now();
  f64 accumulator = 0.0;
  f64 internal_dt = 0.0;
  u32 loop_count = 0;

  time_point frame_last_time = clock::now();
  f64 frame_dt = 0.0;
}

namespace Engine {
  namespace Time {
    void accumulate_time(void)
    {
      time_point current_time = clock::now();
      internal_dt = duration(current_time - last_time).count();
      last_time = current_time;

      accumulator += internal_dt;
      loop_count = 0;
    }

    void update_fixed_step(void)
    {
      time_point frame_current_time = clock::now();
      frame_dt = duration(frame_current_time - frame_last_time).count();
      frame_last_time = frame_current_time;

      accumulator -= get_fixed_dt();
      ++loop_count;
    }

    f64 get_fixed_dt(void)
    {
      return 1.0 / TARGET_FPS;
    }

    f64 get_internal_dt(void)
    {
      return internal_dt;
    }

    f64 get_frame_dt(void)
    {
      return frame_dt;
    }

    bool should_update(void)
    {
      return accumulator >= get_fixed_dt() && loop_count < MAX_UPDATES_PER_FRAME;
    }


  }
};
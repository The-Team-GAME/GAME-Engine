#include "Time.h"

namespace Engine {
  void Time::accumulate_time(void) {
    time_point current_time = clock::now();
    internal_dt = duration(current_time - last_time).count();
    last_time = current_time;

    accumulator += internal_dt;
    loop_count = 0;
  }

  void Time::update_fixed_step(void) {
    time_point frame_current_time = clock::now();
    frame_dt = duration(frame_current_time - frame_last_time).count();
    frame_last_time = frame_current_time;

    accumulator -= get_fixed_dt();
    ++loop_count;
  }

  f64 Time::get_fixed_dt(void)const {
    return 1.0 / target_fps;
  }

  f64 Time::get_internal_dt(void) const {
    return internal_dt;
  }

  f64 Time::get_frame_dt(void) const {
    return frame_dt;
  }

  bool Time::should_update(void)const {
    return accumulator >= get_fixed_dt() && loop_count < loop_cap;
  }
}
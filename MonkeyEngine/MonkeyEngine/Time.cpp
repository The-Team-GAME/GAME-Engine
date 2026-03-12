//-----------------------------------------------------------------------------------
// file:    Time.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   Defines the time system for the engine to keep track of the gameloop 
//          delta time and all the calculations and getters that go with it
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Time.h"
#include "Audio/AudioSystem.h"
#include <chrono>
namespace chrono = std::chrono;

namespace {
  chrono::nanoseconds start_time = chrono::duration_cast<chrono::nanoseconds>((chrono::steady_clock::now()).time_since_epoch());
  chrono::nanoseconds curr_time = start_time;

  f32 delta_time{};                  // in seconds
  f32 tick_rate{ 60.f };            // The tick rate is the number of times you want to game logic to update per second
  f32 fixed_dt{ 1.f / tick_rate };  // precalulate fixed_dt so we dont have to do it every time get_tick_rate is called
  f32 time_scale{ 1.f };            // time scale multiplier
  f32 elapsed_time{};               // total scaled time since engine start
  bool is_paused{ false };          // whether the game is paused (separate from time_scale)
}

namespace Engine {
  namespace Time {
    void update_time(void)
    {
      chrono::nanoseconds new_time = chrono::duration_cast<chrono::nanoseconds>((chrono::steady_clock::now()).time_since_epoch());
      // Convert nanoseconds to seconds
      delta_time = static_cast<f32>((new_time - curr_time).count()) * 1e-9f; // multiply instead of divide because division is veryyy slow
      curr_time = new_time;

      // Accumulate scaled time
      elapsed_time += delta_time * time_scale;
    }

    void set_tick_rate(f32 new_tick_rate)
    {
      tick_rate = new_tick_rate;
      fixed_dt = 1.f / tick_rate;
    }

    // we dont want to allow the user to change the tick rate directly because we also need to recalculate the fixed dt
    f32 get_tick_rate()
    {
      return tick_rate;
    }

    // This returns the fixed dt based on the set framerate
    f32 get_fixed_dt(void)
    {
      return fixed_dt * time_scale;
    }

    f32 get_unscaled_fixed_dt(void)
    {
      return fixed_dt;
    }

    // Returns the delta time per game loop
    // Only to be used to accumulator in the loop
    f32 get_real_dt(void)
    {
      return delta_time;
    }

    // Returns the total scaled elapsed time since engine start
    f32 get_time(void)
    {
      return elapsed_time;
    }

    // Returns the total unscaled elapsed time since engine start
    f32 get_unscaled_time(void)
    {
      chrono::nanoseconds elapsed = curr_time - start_time;
      return static_cast<f32>(elapsed.count()) * 1e-9f;
    }

    f32 calc_fps(f32 dt)
    {
      return 1.f / dt;
    }

    f32 get_time_scale()
    {
      return time_scale;
    }

    void set_time_scale(f32 scale)
    {
      time_scale = scale;
    }

    bool get_paused()
    {
      return is_paused;
    }

    bool set_paused(bool paused)
    {
      if (is_paused == paused) return is_paused; // No change needed

      is_paused = paused;

      // Pause/resume audio when game is paused/unpaused
      if (is_paused)
      {
        Engine::AudioSystem::pauseAll(true);
      }
      else
      {
        Engine::AudioSystem::pauseAll(false);
      }

      return is_paused;
    }

    bool toggle_paused()
    {
      return set_paused(!is_paused);
    }
  }
};
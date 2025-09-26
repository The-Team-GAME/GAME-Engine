#include "Time.h"
#include <chrono>
namespace chrono = std::chrono;

namespace {
	chrono::nanoseconds curr_time = chrono::duration_cast<chrono::nanoseconds>((chrono::steady_clock::now()).time_since_epoch());

	f32 delta_time{}; // in seconds
	f32 tick_rate{ 144.f };
}

namespace Engine {
	namespace Time {
		void update_time(void)
		{
			chrono::nanoseconds new_time = chrono::duration_cast<chrono::nanoseconds>((chrono::steady_clock::now()).time_since_epoch());
			// Convert nanoseconds to seconds
			delta_time = static_cast<f32>((new_time - curr_time).count()) / 1000000000.0f;
			curr_time = new_time;
		}

		f32& get_tick_rate()
		{
			return tick_rate;
		}
		// This calculates the fixed dt based on the set framerate
		f32 get_fixed_dt(void)
		{
			return 1.f / tick_rate;
		}

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_real_dt(void)
		{
			return delta_time;
		}

		f32 calc_fps(f32 dt)
		{
			return 1.f / dt;
		}
	}
};
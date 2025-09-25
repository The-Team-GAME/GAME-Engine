#include "Time.h"
#include <chrono>
namespace chrono = std::chrono;

namespace { //consider making it microseconds
	chrono::milliseconds curr_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());

	f32 delta_time{}; // in seconds
	f32 tick_rate{ 144.f };
}

namespace Engine {
	namespace Time {
		void update_time(void)
		{
			chrono::milliseconds new_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());
			delta_time = static_cast<f32>((new_time - curr_time).count()) / 1000.0;
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
			if (dt == 0.f)
			{
				// 1000 cuz mili seconds so max fps that we can calculate accurately is 1k
				return 1.f / 0.001; 
			}
			return 1.f / dt;
		}
	}
};
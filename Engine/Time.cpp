#include "Time.h"
#include <chrono>

namespace chrono = std::chrono;

namespace { //consider making it microseconds
	chrono::milliseconds curr_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());

	f64 delta_time{}; // in seconds
	s32 frame_rate{ 600 };
}

namespace Engine {
	namespace Time {
		void update_time(void)
		{
			chrono::milliseconds new_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());
			delta_time = static_cast<f64>((new_time - curr_time).count()) / 1000.0;
			curr_time = new_time;
		}

		// This calculates the fixed dt based on the set framerate
		f64 get_dt(void)
		{
			return 1.0 / frame_rate;
		}

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_gameloop_dt(void)
		{
			return static_cast<f32>(delta_time);
		}
	}
};
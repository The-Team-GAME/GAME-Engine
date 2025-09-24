#include "Time.h"
#include <chrono>
namespace chrono = std::chrono;

namespace { //consider making it microseconds
	chrono::milliseconds curr_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());

	f32 delta_time{}; // in seconds
	f32 frame_cap{ 60 };
}

namespace Engine {
	namespace Time {
		void update_time(void)
		{
			chrono::milliseconds new_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());
			delta_time = static_cast<f32>((new_time - curr_time).count()) / 1000.0;
			curr_time = new_time;
		}

		f32& get_frame_cap()
		{
			return frame_cap;
		}
		// This calculates the fixed dt based on the set framerate
		f32 get_fixed_dt(void)
		{
			return 1.f / frame_cap;
		}

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_real_dt(void)
		{
			return delta_time;
		}
	}
};
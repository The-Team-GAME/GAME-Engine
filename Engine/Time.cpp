#include "Time.h"
#include <chrono>

namespace chrono = std::chrono;

namespace { //consider making it microseconds
	chrono::milliseconds curr_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());
	f64 delta_time{}; // in seconds
}

namespace Engine {
	namespace Time {
		void update_time(void)
		{
			chrono::milliseconds new_time = chrono::duration_cast<chrono::milliseconds>((chrono::steady_clock::now()).time_since_epoch());
			delta_time = static_cast<f64>((new_time - curr_time).count()) / 1000.0;
			curr_time = new_time;
		}

		f64 g_delta_time(void) //need to fix also
		{
			return delta_time > 1 / 60.0 ? 1 / 60.0 : delta_time;
		}
	}
};
#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		void update_time(void);

		void set_tick_rate(f32 new_tick_rate);

		f32 get_tick_rate();

		// Returns the delta time to be used during calculations (fixed_dt)
		f32 get_fixed_dt(void);

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_real_dt(void);

		f32 calc_fps(f32 dt);
	}
}
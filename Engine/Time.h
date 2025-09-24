#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		void update_time(void);

		f32& get_frame_cap();

		// Returns the delta time to be used during calculations (fixed_dt)
		f32 get_fixed_dt(void);

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_real_dt(void);

	}
}
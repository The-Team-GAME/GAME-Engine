#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		void update_time(void);

		// Returns the delta time to be used during calculations (fixed_dt)
		f64 get_dt(void);

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_gameloop_dt(void);

	}
}


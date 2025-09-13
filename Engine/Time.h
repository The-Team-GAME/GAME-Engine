#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		static f64 fixed_dt = 1000 / framerate;
		void update_time(void);

		// Returns the delta time to be used during calculations
		f64 get_dt(void);

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f64 get_gameloop_dt(void);

	}
}


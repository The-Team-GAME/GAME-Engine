#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		void update_time(void);

		// Returns the delta time to be used during calculations (fixed_dt)
		f64 get_fixed_dt(void);

		// Returns the delta time per game loop
		// Only to be used to accumulator in the loop
		f32 get_internal_dt(void);

		void reset_accumulator(void);
		bool should_update(void);
		void subtract_fixed_dt(void);

	}
}


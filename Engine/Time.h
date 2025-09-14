#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		/**
		 * @brief Should run at the start of the frame.
		 *
		 * This function calculates the internal delta time since the last frame and updates the accumulator.
		 */
		void begin(void);

		/**
		 * @brief Should run at the end of every successful frame.
		 *
		 * This function decrements the accumulator by the fixed delta time and increments the loop count.
		 */
		void end(void);

		/**
		 * @brief Returns the fixed delta time used for calculations.
		 *
		 * This is the time step for fixed updates, based on the target frame rate.
		 *
		 * @return The fixed delta time in seconds.
		 */
		f64 get_fixed_dt(void);

		/**
		 * @brief Returns the internal delta time per game loop.
		 *
		 * This is the actual time elapsed since the last frame, used only for the accumulator.
		 *
		 * @return The internal delta time in seconds.
		 */
		f64 get_internal_dt(void);

		/**
		 * @brief Checks if a fixed update should occur.
		 *
		 * This function determines if the accumulator has reached the fixed delta time and if the loop count is below the cap.
		 *
		 * @return True if an update should proceed, false otherwise.
		 */
		bool should_update(void); // checks if accumulator >= fixed_dt
	}
}


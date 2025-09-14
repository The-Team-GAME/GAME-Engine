#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		/**
		 * @brief Accumulates time for the game loop's fixed timestep logic.
		 *
		 * This function calculates the time elapsed since the last call and updates
		 * the accumulator used for fixed timestep logic.
		 */
		void accumulate_time(void);

		/**
		 * @brief Updates the time tracking for a single fixed step.
		 *
		 * This function calculates the delta time for the current frame and decrements
		 * the accumulator by the fixed delta time.
		 *
		 * This function is called when a successful frame update occurs.
		 */
		void update_fixed_step(void);

		/**
		 * @brief Gets the fixed delta time.
		 *
		 * This function returns the fixed delta time used for updates, based on the target frames per second (FPS).
		 *
		 * @return The fixed delta time in seconds, calculated as 1 / TARGET_FPS.
		 */
		f64 get_fixed_dt(void);

		/**
		 * @brief Gets the internal delta time.
		 *
		 * This function returns the time elapsed since the last internal update.
		 *
		 * This function should not be used anywhere else.
		 *
		 * @return The time elapsed since the last internal update in seconds.
		 */
		f64 get_internal_dt(void);

		/**
		 * @brief Gets the frame delta time.
		 *
		 * This function returns the delta time for the current frame.
		 *
		 * This is the function that should be used for logic.
		 *
		 * @return The time elapsed for the current frame in seconds.
		 */
		f64 get_frame_dt(void);

		/**
		 * @brief Checks if an update should occur based on the accumulator.
		 *
		 * @return True if the accumulator is greater than or equal to the fixed delta time
		 *         and the loop count is below the maximum allowed updates per frame.
		 */
		bool should_update(void);
	}
}


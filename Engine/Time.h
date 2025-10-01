//-----------------------------------------------------------------------------------
// file:    Time.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   Header file for the time system for the engine to keep track of the gameloop 
//					delta time and all the calculations and getters that go with it
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "TypeDef.h"

namespace Engine {
	namespace Time {

		/*=================================================================================
		Function Brief:
		This function calculates and updates the real delta time between gameloops

		Where to call it:
		This function is to be the first function to be called in the game loop under
		Core::run() as the sunsequent functions like logic, physics or rendering will have
		to rely on the delta time values to accurately perform calculations
		==================================================================================*/
		void update_time(void);

		/*=================================================================================
		Function Brief:
		This function is a mutator to change the tick rate of the game

		Where to call it:
		This function normally should not be called as the tick rate of the game generally
		does not need to be changed. However, for debug purposes, this can be called where
		the programmer intends to mmodify the tick rate
		==================================================================================*/
		void set_tick_rate(f32 new_tick_rate);

		/*=================================================================================
		Function Brief:
		This function is an accessor for the tick rate of the game

		Where to call it:
		This function normally should be called where the user wants to see the current
		tick rate of the game
		==================================================================================*/
		f32 get_tick_rate();

		/*=================================================================================
		Function Brief:
		This function is an accessor for the fixed delta time

		Where to call it:
		This function should be the one that is called whenever delta time calculations
		are required as this engine runs on a fixed_dt time loop for the logic updates
		==================================================================================*/
		f32 get_fixed_dt(void);

		/*=================================================================================
		Function Brief:
		This function is an accessor for the real delta time between gameloops

		Where to call it:
		This function should only be called in Core::run() where it is used to increment
		the accumulator for the fixed_dt time loop and SHOULD NOT BE USED FOR CALCULATIONS
		==================================================================================*/
		f32 get_real_dt(void);

		/*=================================================================================
		Function Brief:
		This function is an accessor for the current fps of the game based off the real_dt

		Where to call it:
		This function normally should be called where the user wants to see the current
		fps(frames per second) of the game
		==================================================================================*/
		f32 calc_fps(f32 dt);
	}
}
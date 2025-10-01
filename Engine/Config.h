//-----------------------------------------------------------------------------------
// file:    Config.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file uses the functions in FileHandler.cpp to load, deserialise and
//					parse the values in the config file into our game engine
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "TypeDef.h"
namespace Config {

	/*=================================================================================
	Function Brief:
	This function loads all the config values found in /Assets/Config/Config.json into 
	the values obtained from the getter functions below.

	Where to call it:
	This function is to be the first function to be called in the constructor for core() 
	as the subsequent functions called might rely on those values and without the values 
	initialised, there might be major errors.
	==================================================================================*/

	void load_config();

	// Accessors for config values (self explainatory)

	// Returns the width that the game window is supposed to have
	s32 get_win_width();

	// returns the height that the game window is supposed to have
	s32 get_win_height();

	// End of Accessors
}


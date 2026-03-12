//-----------------------------------------------------------------------------------
// file:    Config.cpp
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

#include "../Engine/MonkeyEngine.h"

namespace fh = FileHandler;

namespace { // Annonymous/ local namespcae
	const constexpr char* filepath = "Assets/config/";
	const constexpr char* filename = "config.json";
	const constexpr char* container = "config";


	// Config values to be stored
	s32 window_width;
	s32 window_height;
}

namespace Config {

	void load_config()
	{
		fh::set_container(container);
		std::string complete_filepath =  std::string(filepath) + std::string(filename);
		fh::parse_file(complete_filepath);
		if (fh::has_key("win_width"))
		{
			if (fh::get_s32("win_width") != fh::d_s32())
			{
				window_width = fh::get_s32("win_width");
			}
			else { Log::warning("Key: win_width does not contain a value."); }
		}
		else {
			Log::warning("Key: win_width does not exist.");
		}
		if (fh::has_key("win_height"))
		{
			if (fh::get_s32("win_height") != fh::d_s32())
			{
				window_height = fh::get_s32("win_height");
			}
			else { Log::warning("Key: win_height does not contain a value."); }
		}
		else {
			Log::warning("Key: win_height does not exist.");
		}

		fh::clear_json();
	}
	// Accessors for config values
	s32 get_win_width()
	{
		return window_width;
	}

	s32 get_win_height()
	{
		return window_height;
	}
}
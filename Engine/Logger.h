//-----------------------------------------------------------------------------------
// file:    Logger.h
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file defines logging functions for outputting warnings, info, and crash messages.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Log {

	/*=================================================================================
			Function Brief:
			Outputs a warning message to the console. Currently, logging to file is marked as TODO.

			Where to call it:
			Call this function whenever a non-critical issue or potential problem needs to be logged
			for debugging or user notification purposes.
			==================================================================================*/
	void warning(std::string text);

	/*=================================================================================
			Function Brief:
			Outputs an info message to the console. Currently, logging to file is marked as TODO.

			Where to call it:
			Call this function to log general information messages, such as successful operations
			or status updates during program execution.
			==================================================================================*/
	void info(std::string text);

	/*=================================================================================
			Function Brief:
			Outputs a crash message to the console and logs it to a file with a timestamp.
			This function appends the crash details to "crash.log".

			Where to call it:
			Call this function in critical error scenarios where the program is about to terminate
			or when an unrecoverable error occurs, to ensure the crash is recorded.
			==================================================================================*/
	void crash(std::string text);
}


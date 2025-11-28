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
#include <format>
#include <utility>

namespace Log {

	namespace detail {
		void warning_impl(std::string text);
		void info_impl(std::string text);
		void error_impl(std::string text);
		void crash_impl(std::string text);
	}

	/*=================================================================================
			Function Brief:
			Outputs a warning message to the console. Currently, logging to file is marked as TODO.

			Where to call it:
			Call this function whenever a non-critical issue or potential problem needs to be logged
			for debugging or user notification purposes.
			==================================================================================*/
	template<typename... Args>
	void warning(std::format_string<Args...> fmt, Args&&... args) {
#ifndef GAME_BUILD
		detail::warning_impl(std::format(fmt, std::forward<Args>(args)...));
#else
		(void)fmt;
		((void)args, ...);
#endif
	}

	/*=================================================================================
			Function Brief:
			Outputs an info message to the console. Currently, logging to file is marked as TODO.

			Where to call it:
			Call this function to log general information messages, such as successful operations
			or status updates during program execution.
			==================================================================================*/
	template<typename... Args>
	void info(std::format_string<Args...> fmt, Args&&... args) {
#ifndef GAME_BUILD
		detail::info_impl(std::format(fmt, std::forward<Args>(args)...));
#else
		(void)fmt;
		((void)args, ...);
#endif
	}

	/*=================================================================================
			Function Brief:
			Outputs an error message to the console. Currently, logging to file is marked as TODO.

			Where to call it:
			Call this function whenever a critical issue or error needs to be logged
			for debugging or user notification purposes.
			==================================================================================*/
	template<typename... Args>
	void error(std::format_string<Args...> fmt, Args&&... args) {
#ifndef GAME_BUILD
		detail::error_impl(std::format(fmt, std::forward<Args>(args)...));
#else
		(void)fmt;
		((void)args, ...);
#endif
	}

	/*=================================================================================
			Function Brief:
			Outputs a crash message to the console and logs it to a file with a timestamp.
			This function appends the crash details to "crash.log".

			Where to call it:
			Call this function in critical error scenarios where the program is about to terminate
			or when an unrecoverable error occurs, to ensure the crash is recorded.
			==================================================================================*/
	template<typename... Args>
	void crash(std::format_string<Args...> fmt, Args&&... args) {
#ifndef GAME_BUILD
		detail::crash_impl(std::format(fmt, std::forward<Args>(args)...));
#else
		(void)fmt;
		((void)args, ...);
#endif
	}
}


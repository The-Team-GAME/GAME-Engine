//-----------------------------------------------------------------------------------
// file:    Logger.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file implements the logging functions for outputting warnings, info, and crash messages.
//
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Log {
	namespace detail {

		// Outputs a warning message to the console
		void warning_impl(std::string text)
		{
			// Stuff to log file TODO

			// Stuff to cout in the console
			std::cout << "Warning>> " << text << std::endl;
		}

		// Outputs an info message to the console
		void info_impl(std::string text)
		{
			// Stuff to log file TODO

			// Stuff to cout in the console
			std::cout << "Info>> " << text << std::endl;
		}

		// Outputs an error message to the console
		void error_impl(std::string text)
		{
			// Stuff to log file TODO

			// Stuff to cout in the console
			std::cerr << "Error>> " << text << std::endl;
		}

		// Outputs a crash message to the console and logs it to a file with a timestamp
		void crash_impl(std::string text)
		{
			// Get current time using chrono
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

			// Simple timestamp without localtime
			std::stringstream timeStream;
			timeStream << "T+" << seconds;
			std::string timeStr = timeStream.str();

			// Write to crash log file
			std::ofstream logFile("crash.log", std::ios::app);
			if (logFile.is_open())
			{
				logFile << "[" << timeStr << "] CRASH>> " << text << std::endl;
				logFile.close();
			}

			// Also output to console
			std::cerr << "CRASH>> " << text << std::endl;
		}

		// Outputs a memory-specific log message to the console with [MEMORY] prefix
		// Also logs to memory.log file for detailed memory tracking
		void memory_impl(std::string text)
		{
			// Get current time using chrono for precise memory tracking
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

			// Format timestamp
			std::stringstream timeStream;
			timeStream << millis;
			std::string timeStr = timeStream.str();

			// Write to memory log file for persistent tracking
			std::ofstream logFile("memory.log", std::ios::app);
			if (logFile.is_open())
			{
				logFile << "[" << timeStr << "] " << text << std::endl;
				logFile.close();
			}

			// Output to console with special prefix for easy filtering
			std::cout << "[MEMORY] " << text << std::endl;
		}
	}
}
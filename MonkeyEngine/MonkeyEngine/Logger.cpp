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
#include <mutex>
#include <deque>

namespace Log {

	// ── In-memory log buffer ──────────────────────────────────────────
	namespace {
		const size_t MAX_LOG_ENTRIES = 2000;
		std::deque<LogEntry> s_log_buffer;
		std::mutex s_log_mutex;

		auto s_start_time = std::chrono::steady_clock::now();

		void push_entry(const std::string& text, Level level)
		{
			float elapsed = std::chrono::duration<float>(
				std::chrono::steady_clock::now() - s_start_time).count();

			std::lock_guard<std::mutex> lk(s_log_mutex);
			s_log_buffer.push_back({ text, level, elapsed });
			if (s_log_buffer.size() > MAX_LOG_ENTRIES)
				s_log_buffer.pop_front();
		}
	}

	std::deque<LogEntry> get_log_entries()
	{
		std::lock_guard<std::mutex> lk(s_log_mutex);
		return s_log_buffer;
	}

	void clear_log()
	{
		std::lock_guard<std::mutex> lk(s_log_mutex);
		s_log_buffer.clear();
	}

	namespace detail {

		// Outputs a warning message to the console
		void warning_impl(std::string text)
		{
			push_entry(text, Level::Warning);
			std::cout << "Warning>> " << text << std::endl;
		}

		// Outputs an info message to the console
		void info_impl(std::string text)
		{
			push_entry(text, Level::Info);
			std::cout << "Info>> " << text << std::endl;
		}

		// Outputs an error message to the console
		void error_impl(std::string text)
		{
			push_entry(text, Level::Error);
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

			// Also output to console and buffer
			push_entry(text, Level::Crash);
			std::cerr << "CRASH>> " << text << std::endl;
		}
	}
}
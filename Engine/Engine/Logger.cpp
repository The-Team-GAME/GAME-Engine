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

#ifdef GAME_BUILD
		// File logging for GAME_BUILD
		std::ofstream s_log_file;
		std::mutex s_file_mutex;
		bool s_file_opened = false;

		void ensure_file_open() {
			if (!s_file_opened) {
				s_log_file.open("game.log", std::ios::out | std::ios::trunc);
				s_file_opened = true;
			}
		}

		void write_to_file(const std::string& prefix, const std::string& text) {
			std::lock_guard<std::mutex> lk(s_file_mutex);
			ensure_file_open();
			if (s_log_file.is_open()) {
				float elapsed = std::chrono::duration<float>(
					std::chrono::steady_clock::now() - s_start_time).count();
				s_log_file << "[" << std::fixed << std::setprecision(3) << elapsed << "s] "
					<< prefix << " " << text << std::endl;
				s_log_file.flush();
			}
		}
#endif

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

	void close_log_file()
	{
#ifdef GAME_BUILD
		std::lock_guard<std::mutex> lk(s_file_mutex);
		if (s_log_file.is_open()) {
			s_log_file.close();
		}
#endif
	}

	namespace detail {

		// Outputs a warning message to the console
		void warning_impl(std::string text)
		{
			push_entry(text, Level::Warning);
#ifdef GAME_BUILD
			write_to_file("Warning>>", text);
#else
			std::cout << "Warning>> " << text << std::endl;
			std::cout.flush();
#endif
		}

		// Outputs an info message to the console
		void info_impl(std::string text)
		{
			push_entry(text, Level::Info);
#ifdef GAME_BUILD
			write_to_file("Info>>", text);
#else
			std::cout << "Info>> " << text << std::endl;
			std::cout.flush();
#endif
		}

		// Outputs an error message to the console
		void error_impl(std::string text)
		{
			push_entry(text, Level::Error);
#ifdef GAME_BUILD
			write_to_file("Error>>", text);
#else
			std::cerr << "Error>> " << text << std::endl;
			std::cerr.flush();
#endif
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
#ifdef GAME_BUILD
			write_to_file("CRASH>>", text);
#else
			std::cerr << "CRASH>> " << text << std::endl;
#endif
		}
	}
}
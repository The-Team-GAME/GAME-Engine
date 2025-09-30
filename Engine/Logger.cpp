#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Log {

	void warning(std::string text)
	{
		// Stuff to log file TODO

		// Stuff to cout in the console
		std::cout << "Warning>> " << text << std::endl;
	}

	void info(std::string text)
	{
		// Stuff to log file TODO

		// Stuff to cout in the console
		std::cout << "Info>> " << text << std::endl;
	}

	void crash(std::string text)
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
}
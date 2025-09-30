#include "Logger.h"
#include <iostream>
#include <fstream>
#include <ctime>

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
		// Get current time
		std::time_t now = std::time(nullptr);
		char timeStr[100];
		std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

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
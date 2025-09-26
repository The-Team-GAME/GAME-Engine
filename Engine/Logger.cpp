#include "Logger.h"
#include <iostream>

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
}
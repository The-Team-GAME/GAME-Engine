#pragma once
#include <string>
#include <iostream>

namespace Log {

	void warning(std::string text)
	{
		// Stuff to log file TODO

		// Stuff to cout in the console
		std::cout << "Warning>> " << "text" << std::endl;
	}

	void Info(std::string text)
	{
		// Stuff to log file TODO

		// Stuff to cout in the console
		std::cout << "Info>> " << "text" << std::endl;
	}
}


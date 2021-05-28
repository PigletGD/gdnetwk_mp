#pragma once
#include <iostream>
#include <string>
#include <sstream>

namespace Input {
	std::string getStringInput() {
		std::string input;

		// Get input
		std::getline(std::cin, input);

		// Clean input
		auto returnCharItr = input.find('\r');
		auto newlineCharItr = input.find('\n');
		if (returnCharItr != std::string::npos)
			input = input.substr(returnCharItr);
		if (newlineCharItr != std::string::npos)
			input = input.substr(newlineCharItr);

		return input;
	}

	char getCharInput() {
		std::string input;

		// Get input
		std::getline(std::cin, input);

		return input[0];
	}

	int getIntInput() {
		int value;
		std::string input;

		// Get input
		std::getline(std::cin, input);

		// Get only the first integer
		std::stringstream inputStream;
		inputStream << input;
		inputStream >> value;

		return value;
	}
}

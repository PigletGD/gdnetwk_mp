#pragma once
#include <random>

namespace Random {
	std::default_random_engine generator;
	
	void seed(int value) {
		generator.seed(value);
	}

	int getRandomInt(int inclusiveMin, int inclusiveMax) {
		std::uniform_int_distribution<int> distribution(inclusiveMin, inclusiveMax);
		return distribution(generator);
	}
}

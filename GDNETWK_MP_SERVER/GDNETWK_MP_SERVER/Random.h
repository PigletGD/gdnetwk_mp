#pragma once
#include <random>

class Random
{
public:
	static void seed(long long value)
	{
		generator.seed(value);
	}

	static int getRandom(int inclusiveMin, int inclusiveMax)
	{
		std::uniform_int_distribution dist(inclusiveMin, inclusiveMax);
		return dist(generator);
	}

	static float getRandom(float inclusiveMin, float inclusiveMax)
	{
		std::uniform_real_distribution dist(inclusiveMin, inclusiveMax);
		return dist(generator);
	}

private:
	static std::default_random_engine generator;
};

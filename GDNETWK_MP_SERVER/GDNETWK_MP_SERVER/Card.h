#pragma once
#include <cstdint>
#include <iostream>

class Card {
public:
	Card(uint32_t value)
		: value(value)
	{
		if (value <= 1 || value >= 12)
		{
			std::cerr << "Invalid card value: " << value;
		}
	}

	~Card() = default;

	uint32_t getValue() const { return value; }

private:
	uint32_t value;
};

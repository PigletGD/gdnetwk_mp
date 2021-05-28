#pragma once
#include <vector>

class Hand {
public:
	Hand() = default;
	~Hand() = default;

	void addCard(Card card) {
		cards.push_back(card);
	}

	void clear() {
		cards.clear();
	}

	const std::vector<Card>& getCards() const {
		return cards;
	}

	uint32_t getTotalValue() const {
		uint32_t totalValue = 0;
		uint32_t aceCardsCount = 0;

		for (const Card& card : cards) {
			uint32_t cardValue = card.getValue();

			totalValue += cardValue;

			if (cardValue == 11) {
				aceCardsCount++;
			}

			if (totalValue > 21 && aceCardsCount > 0) {
				totalValue -= 10;
				aceCardsCount--;
			}
		}

		return totalValue;
	}

private:
	std::vector<Card> cards;
};

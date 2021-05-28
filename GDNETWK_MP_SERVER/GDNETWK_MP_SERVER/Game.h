#pragma once
#include "Card.h"
#include "Hand.h"
#include "Player.h"
#include "Input.h"
#include "Random.h"
#include <vector>

enum class PlayerState {
	None,
	NotPlaying,
	TakingTurn,
	DoneTakingTurn,
	Bust,
	Win,
	Push,
	Blackjack
};

class Game {
public:
	Game() {
		resetDeck();
		numPlayers = 0;
	}

	~Game() = default;

	void printSeparator() {
		std::cout << "\n\n------------------\n\n";
	}

	void run() {

		std::cout << "START GAME" << std::endl;
		int time = 0;

		while (joinedPlayers.size() < 4) {
			/*if (time > 100000) {
				time -= 100000;
				std::cout << "CURRENT PLAYER COUNT" <<  << std::endl;
			}*/
		}

		while (true) {
			reset();

			for (int i = 0; i < joinedPlayers.size(); i++) {
				int& player_bet = playerBets[i];
				
				while (player_bet <= 0) {
					std::cout << "\nPlace your bet (0 if you want to skip): ";
					player_bet = Input::getIntInput();
				}
				
				if (player_bet == 0) {
					playerStates[i] = PlayerState::NotPlaying;
					std::cout << "\nSkipping this game...";
				} else {
					playerStates[i] = PlayerState::TakingTurn;
				}
			}

			dealStartingCards();

			for (int i = 0; i < joinedPlayers.size(); i++) {
				PlayerState& currentPlayerState = playerStates[i];

				// Skip not playing player
				if (currentPlayerState == PlayerState::NotPlaying) {
					continue;
				}

				// Check for blackjack
				if (playerHands[i].getTotalValue() == 21) {
					currentPlayerState = PlayerState::Blackjack;
					continue;
				}
				
				printSeparator();

				// Process player's turn
				while (currentPlayerState == PlayerState::TakingTurn) {
					printDealerCardsInGame();
					printPlayerState(i);
					
					std::cout << "Press H to draw a card, press S to stand: ";
					char move = Input::getCharInput();

					if (move == 'h' || move == 'H') {
						hit(i);
					} else if (move == 's' || move == 'S') {
						stand(i);
					} else {
						std::cout << "Invalid input! Please try again.\n";
					}
					
					std::cout << "\n\n";
				}
			}

			drawDealerCards();
			calculateGameEndResults();
			
			printSeparator();
			printDealerCardsEndGame();
			printPlayerResults();
		}
	}

	void addPlayer(const Player& player) {
		joinedPlayers.push_back(player);
		playerHands.emplace_back();
		playerBets.push_back(0);
		playerStates.emplace_back(PlayerState::None);

		std::cout << "Added " << player.getClientNumber() << ", Player Count At " << joinedPlayers.size() << std::endl;
	}

private:
	void dealStartingCards() {
		// Draw cards for dealer
		dealerHand.addCard(drawRandomCardFromDeck());
		dealerHand.addCard(drawRandomCardFromDeck());

		// Draw cards for players
		for (Hand& hand : playerHands) {
			hand.addCard(drawRandomCardFromDeck());
			hand.addCard(drawRandomCardFromDeck());
		}
	}

	Card drawRandomCardFromDeck() {
		int index = Random::getRandom(0, deck.size() - 1);
		Card card = deck[index];
		deck.erase(deck.begin() + index);
		return card;
	}

	void hit(int i) {
		Hand& hand = playerHands[i];
		
		hand.addCard(drawRandomCardFromDeck());

		if (hand.getTotalValue() > 21) {
			playerStates[i] = PlayerState::Bust;
			std::cout << "\n\n******You went bust!******\n";
		}
	}

	void stand(int i) {
		playerStates[i] = PlayerState::DoneTakingTurn;
	}

	void drawDealerCards()
	{
		// Dealer draws until 17 or greater
		while (dealerHand.getTotalValue() < 17) {
			dealerHand.addCard(drawRandomCardFromDeck());
		}
	}

	void calculateGameEndResults() {
		int dealerHandValue = dealerHand.getTotalValue();
		
		for (int i = 0; i < joinedPlayers.size(); i++) {
			const Hand& playerHand = playerHands[i];
			const int& playerBet = playerBets[i];
			Player& player = joinedPlayers[i];
			PlayerState& playerGameResult = playerStates[i];

			// If player did not blackjack or bust
			if (playerGameResult == PlayerState::DoneTakingTurn) {
				int playerHandValue = playerHand.getTotalValue();

				if (dealerHandValue > 21 || playerHandValue > dealerHandValue) {
					playerGameResult = PlayerState::Win;
				} else if (playerHandValue == dealerHandValue) {
					playerGameResult = PlayerState::Push;
				} else {
					playerGameResult = PlayerState::Bust;
				}
			}

			// Calculate money of player
			if (playerGameResult == PlayerState::Bust) {
				player.setMoney(player.getMoney() - playerBet);
			} else if (playerGameResult == PlayerState::Win) {
				player.setMoney(player.getMoney() + playerBet);
			} else if (playerGameResult == PlayerState::Blackjack) {
				player.setMoney(player.getMoney() + playerBet * 1.5f);
			}
		}
	}

	void printPlayerState(int i) const {
		const Player& player = joinedPlayers[i];
		const Hand& hand = playerHands[i];
		const int& bet = playerBets[i];

		std::cout << "Player: " << player.getName() << '\n';
		std::cout << "Money: " << player.getMoney() << '\n';
		std::cout << "Your bet: " << bet << '\n';

		std::cout << "Your cards: ";
		for (int i = 0; i < hand.getCards().size() - 1; i++) {
			const Card& card = hand.getCards()[i];

			std::cout << card.getValue() << ", ";
		}
		std::cout << hand.getCards()[hand.getCards().size() - 1].getValue() << '\n';
		std::cout << "Total value: " << hand.getTotalValue() << '\n';
	}

	void printDealerCardsInGame() const {
		std::cout << "Dealer's cards: " << dealerHand.getCards()[0].getValue() << " ?\n";
	}

	void printDealerCardsEndGame() const {
		const auto& dealerCards = dealerHand.getCards();
		
		std::cout << "Dealer's final cards: ";
		for (int i = 0; i < dealerCards.size() - 1; i++) {
			std::cout << dealerCards[i].getValue() << ", ";
		}
		std::cout << dealerCards[dealerCards.size() - 1].getValue() << '\n';
		std::cout << "Dealer's total value: " << dealerHand.getTotalValue() << '\n';
	}

	void printPlayerResults() const {
		for (int i = 0; i < joinedPlayers.size(); i++) {
			const Player& player = joinedPlayers[i];
			const PlayerState& result = playerStates[i];

			std::cout << "Player: " << player.getName() << '\n';
			std::cout << "Money: " << player.getMoney() << '\n';
			std::cout << ((result == PlayerState::Win) ? "You win!\n" : "You lost!\n");
		}
	}

	void reset() {
		resetDeck();
		clearHands();
		clearBets();
		clearGameResults();
	}

	void resetDeck() {
		for (int i = 0; i < 4; i++) {
			deck.emplace_back(11);
		}

		for (int i = 2; i <= 9; i++) {
			deck.emplace_back(i);
			deck.emplace_back(i);
			deck.emplace_back(i);
			deck.emplace_back(i);
		}

		for (int i = 0; i < 16; i++) {
			deck.emplace_back(10);
		}
	}

	void clearHands() {
		for (Hand& playerHands : playerHands) {
			playerHands.clear();
		}
		dealerHand.clear();
	}

	void clearBets() {
		for (int i = 0; i < playerBets.size(); i++) {
			playerBets[i] = 0;
		}
	}

	void clearGameResults() {
		for (PlayerState& gameResult : playerStates) {
			gameResult = PlayerState::None;
		}
	}

	const int MAX_PLAYERS = 4;

	std::vector<Card> deck;
	std::vector<Player> joinedPlayers;
	std::vector<Player> queuedPlayersToJoin;
	std::vector<PlayerState> playerStates;
	std::vector<Hand> playerHands;
	std::vector<int> playerBets;
	Hand dealerHand;
	int numPlayers;
};

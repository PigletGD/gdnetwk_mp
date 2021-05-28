#pragma once
#include "Card.h"
#include "Hand.h"
#include "Player.h"
#include "Input.h"
#include "Random.h"
#include "TcpListener.h"
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
	Game(TcpListener* listener) {
		listener->setCallback(std::bind(&Game::Listener_MessageReceived, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		resetDeck();
		numPlayers = 0;
	}

	~Game() = default;

	void printSeparator() {
		std::cout << "\n\n------------------\n\n";
	}

	void run() {

		gameStarted = false;

		std::cout << "START GAME" << std::endl;

		while (joinedPlayers.size() < 2);

		gameStarted = true;

		while (true) {
			reset();

			allowPlayersToBet();

			dealStartingCards();

			givePlayersTurns();

			drawDealerCards();
			calculateGameEndResults();
			
			printSeparator();
			printDealerCardsEndGame();
			printPlayerResults();

			//moveQueuedPlayersToGame();
		}
	}

	void addPlayer(const Player& player) {
		joinedPlayers.push_back(player);
		playerHands.emplace_back();
		playerBets.push_back(0);
		playerStates.emplace_back(PlayerState::None);

		std::cout << "Added " << player.getName() << ", Player Count At " << joinedPlayers.size() << std::endl;
	}

	void addPlayerToQueue(const Player& player) {
		queuedPlayersToJoin.push_back(player);

		std::cout << "Added " << player.getName() << " to queue, Queue Count At " << queuedPlayersToJoin.size() << std::endl;
	}

	void moveQueuedPlayersToGame() {
		for (int i = 0; i < queuedPlayersToJoin.size(); i++) {
			addPlayer(queuedPlayersToJoin[i]);
		}

		queuedPlayersToJoin.clear();

		std::cout << "Moved queued to active players" << std::endl;
	}

private:
	void allowPlayersToBet() {
		playerTurn = 0;

		std::cout << joinedPlayers.size() << std::endl;

		while (playerTurn < joinedPlayers.size()) {
			std::cout << std::endl << joinedPlayers[playerTurn].getName() << ", place your bet (0 if you want to skip): ";

			lockFunction();

			switchPlayerTurn();
		}
	}

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

	void givePlayersTurns() {
		playerTurn = 0;

		while (playerTurn < joinedPlayers.size()) {
			PlayerState& currentPlayerState = playerStates[playerTurn];

			// Skip not playing player
			if (currentPlayerState == PlayerState::NotPlaying) {
				std::cout << "not playing" << std::endl;
				switchPlayerTurn();
				continue;
			}

			// Check for blackjack
			if (playerHands[playerTurn].getTotalValue() == 21) {
				std::cout << "blackjack" << std::endl;
				currentPlayerState = PlayerState::Blackjack;
				switchPlayerTurn();
				continue;
			}

			printSeparator();

			// Process player's turn
			while (currentPlayerState == PlayerState::TakingTurn) {
				printDealerCardsInGame();
				printPlayerState(playerTurn);

				std::cout << "Type \\hit or \\h to hit or \\stand or \\s to stand: ";

				lockFunction();
			}

			switchPlayerTurn();
		}
	}

	void hit(int i) {
		Hand& hand = playerHands[i];
		
		hand.addCard(drawRandomCardFromDeck());

		if (hand.getTotalValue() > 21) {
			playerStates[i] = PlayerState::Bust;
			std::cout << "hand value: " << hand.getTotalValue() << "\n******You went bust!******\n";
		}
		else if (hand.getTotalValue() == 21) {
			playerStates[i] = PlayerState::Blackjack;
		}

		unlockFunction();
	}

	void stand(int i) {
		playerStates[i] = PlayerState::DoneTakingTurn;
		unlockFunction();
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

	void Listener_MessageReceived(TcpListener* listener, int client, int sentClient, std::string msg) {
		std::cout << "processing message" << std::endl;

		std::string originalMsg = msg;
		std::string delimiter = " ";
		size_t pos = msg.find(delimiter);
		msg.erase(0, pos + delimiter.size());
		pos = msg.find(delimiter);
		std::string command = msg.substr(0, pos);

		if (command == "\\add") {
			msg.erase(0, pos + delimiter.size());
			pos = msg.find(delimiter);

			for (int i = 0; i < joinedPlayers.size(); i++) {
				if (sentClient == joinedPlayers[i].getClientNumber())
				{
					if (originalMsg[0] == ' ' && client != sentClient) {
						std::cout << "hi" << std::endl;
						std::string sendMessage = msg.substr(0, pos) + " joined the Game!\r";
						listener->sendMessageToClient(client, sendMessage);
						return;
					}
					else return;
				}	
			}

			if (client == sentClient) {
				Player newPlayer(msg.substr(0, pos), client, 1000);
				
				if (!gameStarted) addPlayer(newPlayer);
				else addPlayerToQueue(newPlayer);
			}
		}
		else if (command == "\\bet") {
			std::cout << "betting command" << std::endl;
			if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
				sentClient == client && playerStates[playerTurn] == PlayerState::None) {

				msg.erase(0, pos + delimiter.size());
				pos = msg.find(delimiter);

				std::string player_bet_string = msg.substr(0, pos);
				if ((int)player_bet_string[0] >= 48 && (int)player_bet_string[0] <= 57) {
					int player_bet = std::stoi(player_bet_string);

					if (player_bet <= 0) {
						playerStates[playerTurn] = PlayerState::NotPlaying;
						std::cout << "\nSkipping this game...";
					}
					else {
						playerBets[playerTurn] = player_bet;
						playerStates[playerTurn] = PlayerState::TakingTurn;
					}

					unlockFunction();
				}
			}
			std::cout << "betting command end" << std::endl;
		}
		else if (command == "\\hit" || command == "\\h") {
			if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
				sentClient == client && playerStates[playerTurn] == PlayerState::TakingTurn) {
				hit(playerTurn);

				std::cout << std::endl;
			}
		}
		else if (command == "\\stand" || command == "\\s") {
			if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
				sentClient == client && playerStates[playerTurn] == PlayerState::TakingTurn) {
				stand(playerTurn);

				std::cout << std::endl;
			}
		}
		else {
			if (client == sentClient) return;
			listener->sendMessageToClient(client, originalMsg);
		} 
	}

	void switchPlayerTurn() {
		std::cout << "increment turn" << std::endl;
		playerTurn++;
	}

	void lockFunction() {
		lock = true;
		std::cout << "\nlock" << std::endl;
		while (lock);
		std::cout << "unlock" << std::endl;
	}

	void unlockFunction() {
		lock = false;
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
	int playerTurn;
	bool lock;
	bool gameStarted;
};

#pragma once
#include "Card.h"
#include "Hand.h"
#include "Player.h"
#include "Input.h"
#include "Random.h"
#include "TcpListener.h"
#include <vector>
#include <semaphore>

enum class PlayerState {
	None,
	NotPlaying,
	TakingTurn,
	DoneTakingTurn,
	Bust,
	Win,
	Push,
	Blackjack,
	Quitting
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
		playerTurn = 0;

		std::cout << "START GAME" << std::endl;

		while (joinedPlayers.size() < 2);

		gameStarted = true;

		while (true) {
			reset();

			allowPlayersToBet();
			std::cout << "deal starting cards\n";
			dealStartingCards();
			std::cout << "give player turns\n";
			givePlayersTurns();

			std::cout << "draw dealer cards\n";
			drawDealerCards();
			std::cout << "calculate end resulst\n";
			calculateGameEndResults();
			
			printSeparator();
			std::cout << "print dealer results\n";
			printDealerCardsEndGame();
			std::cout << "print player results\n";
			printPlayerResults();

			moveQueuedPlayersToGame();
			removeQuittedPlayers();

			if (verifyIfGameIsEmpty())
				break;
		}

		std::cout << "game has ended" << std::endl;
		exit(0);
	}

	void addPlayer(const Player& player) {
		mutex.acquire();
		joinedPlayers.push_back(player);
		playerHands.emplace_back();
		playerBets.push_back(0);
		playerStates.emplace_back(PlayerState::None);

		std::cout << "Added " << player.getName() << ", Player Count At " << joinedPlayers.size() << std::endl;
		mutex.release();
	}

	void addPlayerToQueue(const Player& player) {
		mutex.acquire();
		queuedPlayersToJoin.push_back(player);

		std::cout << "Added " << player.getName() << " to queue, Queue Count At " << queuedPlayersToJoin.size() << std::endl;
		mutex.release();
	}

	void moveQueuedPlayersToGame() {
		for (int i = 0; i < queuedPlayersToJoin.size(); i++) {
			addPlayer(queuedPlayersToJoin[i]);
		}

		mutex.acquire();
		queuedPlayersToJoin.clear();
		mutex.release();

		std::cout << "Moved queued to active players" << std::endl;
	}

	void queuePlayerToRemove(int client) {
		mutex.acquire();
		for (int i = 0; i < queuedPlayersToJoin.size(); i++) {
			if (client == queuedPlayersToJoin[i].getClientNumber()) {
				queuedPlayersToJoin.erase(queuedPlayersToJoin.begin() + i);
				std::cout << "Remove " << client << std::endl;
				mutex.release();
				return;
			}
		}

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (client == joinedPlayers[i].getClientNumber()) {
				clientsQuitting.push_back(client);
				playerStates[i] = PlayerState::Quitting;
				if (i == playerTurn) unlockFunction();
				std::cout << "Queue Player " << joinedPlayers[i].getName() << " Removal" << std::endl;
				mutex.release();
				return;
			}
		}
		mutex.release();
	}

	void removeQuittedPlayers() {
		mutex.acquire();
		for (int i = 0; i < clientsQuitting.size(); i++) {
			for (int j = 0; j < joinedPlayers.size(); j++) {
				if (clientsQuitting[i] == joinedPlayers[j].getClientNumber()) {
					std::cout << "Removing Player " << joinedPlayers[j].getName() << std::endl;
					clientsQuitting.erase(clientsQuitting.begin() + i);
					joinedPlayers.erase(joinedPlayers.begin() + j);
					playerStates.erase(playerStates.begin() + j);
					playerHands.erase(playerHands.begin() + j);
					playerBets.erase(playerBets.begin() + j);
					i--; j++;
				}
			}
		}
		mutex.release();
	}

private:
	void allowPlayersToBet() {
		while (playerTurn < joinedPlayers.size()) {
			std::cout << std::endl << joinedPlayers[playerTurn].getName() << ", place your bet (0 if you want to skip): ";

			if (playerStates[playerTurn] != PlayerState::Quitting) lockFunction();

			if (cycleThroughPlayerTurns()) break;
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
		mutex.acquire();
		int index = Random::getRandom(0, deck.size() - 1);
		Card card = deck[index];
		deck.erase(deck.begin() + index);
		mutex.release();
		return card;
	}

	void givePlayersTurns() {
		while (playerTurn < joinedPlayers.size()) {
			mutex.acquire();
			PlayerState& currentPlayerState = playerStates[playerTurn];

			// Quitting
			if (currentPlayerState == PlayerState::Quitting) {
				std::cout << "quitting soon" << std::endl;
				mutex.release();
				if (cycleThroughPlayerTurns()) break;
				else continue;
			}

			// Skip not playing player
			if (currentPlayerState == PlayerState::NotPlaying) {
				std::cout << "not playing" << std::endl;
				mutex.release();
				if (cycleThroughPlayerTurns()) break;
				else continue;
			}

			// Check for blackjack
			if (playerHands[playerTurn].getTotalValue() == 21) {
				std::cout << "blackjack" << std::endl;
				currentPlayerState = PlayerState::Blackjack;
				mutex.release();
				if (cycleThroughPlayerTurns()) break;
				else continue;
			}

			printSeparator();
			mutex.release();

			// Process player's turn
			while (currentPlayerState == PlayerState::TakingTurn) {
				printDealerCardsInGame();
				printPlayerState(playerTurn);

				std::cout << "Type \\hit or \\h to hit or \\stand or \\s to stand: ";

				lockFunction();
			}

			if (cycleThroughPlayerTurns()) break;
		}
	}

	void hit(int i) {
		mutex.acquire();
		Hand& hand = playerHands[i];
		mutex.release();
		
		hand.addCard(drawRandomCardFromDeck());

		mutex.acquire();
		if (playerStates[i] != PlayerState::Quitting) {
			if (hand.getTotalValue() > 21) {
				playerStates[i] = PlayerState::Bust;
				std::cout << "hand value: " << hand.getTotalValue() << "\n******You went bust!******\n";
			}
			else if (hand.getTotalValue() == 21) {
				playerStates[i] = PlayerState::Blackjack;
			}
		}
		mutex.release();

		unlockFunction();
	}

	void stand(int i) {
		mutex.acquire();
		if (playerStates[i] != PlayerState::Quitting)
		playerStates[i] = PlayerState::DoneTakingTurn;
		mutex.release();
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
		mutex.acquire();
		int dealerHandValue = dealerHand.getTotalValue();
		
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] == PlayerState::Quitting) continue;
			
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
		mutex.release();
	}

	void printPlayerState(int i) {
		mutex.acquire();
		if (playerStates[i] == PlayerState::Quitting) {
			mutex.release();
			return;
		}

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
		mutex.release();
	}

	void printDealerCardsInGame() {
		mutex.acquire();
		std::cout << "Dealer's cards: " << dealerHand.getCards()[0].getValue() << " ?\n";
		mutex.release();
	}

	void printDealerCardsEndGame() {
		mutex.acquire();
		const auto& dealerCards = dealerHand.getCards();
		
		std::cout << "Dealer's final cards: ";
		for (int i = 0; i < dealerCards.size() - 1; i++) {
			std::cout << dealerCards[i].getValue() << ", ";
		}
		std::cout << dealerCards[dealerCards.size() - 1].getValue() << '\n';
		std::cout << "Dealer's total value: " << dealerHand.getTotalValue() << '\n';
		mutex.release();
	}

	void printPlayerResults() {
		mutex.acquire();
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] == PlayerState::Quitting) continue;
			const Player& player = joinedPlayers[i];
			const PlayerState& result = playerStates[i];

			std::cout << "Player: " << player.getName() << '\n';
			std::cout << "Money: " << player.getMoney() << '\n';
			std::cout << ((result == PlayerState::Win) ? "You win!\n" : "You lost!\n");
		}
		mutex.release();
	}

	void reset() {
		resetDeck();
		clearHands();
		clearBets();
		clearGameResults();
	}

	void resetDeck() {
		mutex.acquire();
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
		mutex.release();
	}

	void clearHands() {
		mutex.acquire();
		for (Hand& playerHands : playerHands) {
			playerHands.clear();
		}
		dealerHand.clear();
		mutex.release();
	}

	void clearBets() {
		mutex.acquire();
		for (int i = 0; i < playerBets.size(); i++) {
			playerBets[i] = 0;
		}
		mutex.release();
	}

	void clearGameResults() {
		mutex.acquire();
		for (int i = 0; i < playerStates.size(); i++) {
			playerStates[i] = PlayerState::None;
		}
		mutex.release();
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

			// player join message
			//   every player except one joining
			//   

			mutex.acquire();
			if (originalMsg[0] == ' ') {
				if (client == sentClient) {
					Player newPlayer(msg.substr(0, pos), client, 1000);
					mutex.release();

					if (!gameStarted) addPlayer(newPlayer);
					else addPlayerToQueue(newPlayer);
				}
				else {
					std::cout << "hi" << std::endl;
					std::string sendMessage = msg.substr(0, pos) + " joined the Game!\r";
					listener->sendMessageToClient(client, sendMessage);
					mutex.release();
				}
			}
			else mutex.release();
		}
		else if (command == "\\bet") {
			std::cout << "betting command" << std::endl;
			mutex.acquire();
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
			mutex.release();
		}
		else if (command == "\\hit" || command == "\\h") {
			mutex.acquire();
			if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
				sentClient == client && playerStates[playerTurn] == PlayerState::TakingTurn) {
				mutex.release();

				hit(playerTurn);

				std::cout << std::endl;

				mutex.acquire();
			}
			mutex.release();
		}
		else if (command == "\\stand" || command == "\\s") {
			mutex.acquire();
			if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
				sentClient == client && playerStates[playerTurn] == PlayerState::TakingTurn) {
				mutex.release();

				stand(playerTurn);

				std::cout << std::endl;

				mutex.acquire();
			}
			mutex.release();
		}
		else {
			if (client == sentClient) return;
			listener->sendMessageToClient(client, originalMsg);
		} 
	}

	bool cycleThroughPlayerTurns() {
		std::cout << "increment turn" << std::endl;
		bool endPhase = false;
		mutex.acquire();
		playerTurn++;

		if (playerTurn >= joinedPlayers.size()) {
			playerTurn = 0;
			endPhase = true;
		}
		mutex.release();

		return endPhase;
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

	bool verifyIfGameIsEmpty() {
		return joinedPlayers.empty();
	}

	const int MAX_PLAYERS = 4;

	TcpListener* m_Listener;
	std::vector<Card> deck;
	std::vector<Player> joinedPlayers;
	std::vector<Player> queuedPlayersToJoin;
	std::vector<PlayerState> playerStates;
	std::vector<Hand> playerHands;
	std::vector<int> playerBets;
	std::vector<int> clientsQuitting;
	Hand dealerHand;
	int numPlayers;
	int playerTurn;
	bool lock;
	bool gameStarted;
	std::binary_semaphore mutex = std::binary_semaphore(1);
};

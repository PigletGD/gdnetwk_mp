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
	Game(TcpListener* listener) : m_Listener(listener) {
		listener->setCallback(std::bind(&Game::Listener_MessageReceived, this, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

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

		while (!gameStarted);

		gameStarted = true;

		while (true) {
			reset();

			allowPlayersToBet();
			dealStartingCards();
			givePlayersTurns();

			drawDealerCards();
			calculateGameEndResults();
			
			printDealerCardsEndGame();
			printPlayerResults();

			removeQuittedPlayers();
			moveQueuedPlayersToGame();

			if (verifyIfGameIsEmpty())
				break;
		}

		std::cout << "GAME ENDED" << std::endl;
		exit(0);
	}

	void addPlayer(const Player& player) {
		mutex.acquire();
		joinedPlayers.push_back(player);
		playerHands.emplace_back();
		playerBets.push_back(0);
		playerStates.emplace_back(PlayerState::None);
		playerMessages.emplace_back();

		std::string msg = "\n[BLACKJACK] Added " + player.getName() + ", Player Count At ";
		msg += std::to_string(joinedPlayers.size()) + "\n";
		
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting) {
				m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
			}
		}
		mutex.release();
	}

	void addPlayerToQueue(const Player& player) {
		mutex.acquire();
		queuedPlayersToJoin.push_back(player);

		std::string msg = "\n[BLACKJACK] Added " + player.getName() + " to queue, Queue Count At ";
		msg += std::to_string(queuedPlayersToJoin.size()) + "\n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting) {
				m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
			}
		}
		mutex.release();
	}

	void moveQueuedPlayersToGame() {
		std::string msg;
		std::vector<Player> tempPlayerVector;

		for (int i = 0; i < queuedPlayersToJoin.size(); i++) {
			if (joinedPlayers.size() >= MAX_PLAYERS) {
				msg += "\n[BLACKJACK] Max players reached! Placing " + queuedPlayersToJoin[i].getName() + " back in queue";
				tempPlayerVector.push_back(queuedPlayersToJoin[i]);
			}
			else {
				msg += "\n[BLACKJACK] Moved " + queuedPlayersToJoin[i].getName() + " to active players";
				addPlayer(queuedPlayersToJoin[i]);
			}
		}

		mutex.acquire();
		queuedPlayersToJoin.clear();

		for (int i = 0; i < tempPlayerVector.size(); i++) {
			queuedPlayersToJoin.push_back(tempPlayerVector[i]);
		}

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void queuePlayerToRemove(int client) {
		mutex.acquire();

		std::string msg;
		for (int i = 0; i < queuedPlayersToJoin.size(); i++) {
			if (client == queuedPlayersToJoin[i].getClientNumber()) {
				queuedPlayersToJoin.erase(queuedPlayersToJoin.begin() + i);
				msg += "\n[BLACKJACK] Queued to remove queued client " + std::to_string(client) + '\n';
				for (int i = 0; i < joinedPlayers.size(); i++) {
					if (playerStates[i] != PlayerState::Quitting) {
						m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
					}
				}
				mutex.release();
				return;
			}
		}

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (client == joinedPlayers[i].getClientNumber()) {
				clientsQuitting.push_back(client);
				
				if (!gameStarted) {
					msg += "\n[BLACKJACK] Queue Player " + joinedPlayers[i].getName() + " Removal\n";
					for (int i = 0; i < joinedPlayers.size(); i++) {
						if (playerStates[i] != PlayerState::Quitting) {
							m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
						}
					}
					mutex.release();
					removeQuittedPlayers();
				}
				else {
					playerStates[i] = PlayerState::Quitting;
					if (i == playerTurn) unlockFunction();
					msg += "\n[BLACKJACK] Queue Player " + joinedPlayers[i].getName() + " Removal\n";
					for (int i = 0; i < joinedPlayers.size(); i++) {
						if (playerStates[i] != PlayerState::Quitting) {
							m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
						}
					}
					mutex.release();
				}
				return;
			}
		}
		mutex.release();
	}

	void removeQuittedPlayers() {
		mutex.acquire();

		std::string msg = "\n[BLACKJACK]\n";

		for (int i = 0; i < clientsQuitting.size(); i++) {
			for (int j = 0; j < joinedPlayers.size(); j++) {
				if (clientsQuitting[i] == joinedPlayers[j].getClientNumber()) {
					msg += "Removing Player " + joinedPlayers[j].getName() + "\n";
					clientsQuitting.erase(clientsQuitting.begin() + i);
					joinedPlayers.erase(joinedPlayers.begin() + j);
					playerStates.erase(playerStates.begin() + j);
					playerHands.erase(playerHands.begin() + j);
					playerBets.erase(playerBets.begin() + j);
					playerMessages.erase(playerMessages.begin() + j);
					i--; j++;
				}
			}
		}

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

private:
	void allowPlayersToBet() {
		while (!joinedPlayers.empty() && playerTurn < joinedPlayers.size()) {
			
			sendPlayersBetMessage();
			
			if (playerStates[playerTurn] != PlayerState::Quitting) lockFunction();

			sendInputtedBetMessage();

			if (cycleThroughPlayerTurns()) break;
		}
	}

	void sendPlayersBetMessage() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + ", place your bet (0 if you want to skip)\n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void sendInputtedBetMessage() {
		mutex.acquire();
		std::string msg;

		if (playerBets[playerTurn] <= 0)
			msg += "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + " is skipping this round...\n";
		else 
		{
			msg += "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + " betted ";
			msg += std::to_string(playerBets[playerTurn]) + "\n";
		}
		
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
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

	void sendQuitMessage() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + " is quitting... Skipping turn...\n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void sendPlayerSkippingTurn() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + " is not playing this turn...\n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void sendPlayersGotBlackjack() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + " got blackjack!!!\n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void sendPlayersTurnInstructions() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + ", type \\hit or \\h to hit or \\stand or \\s to stand \n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void givePlayersTurns() {
		while (!joinedPlayers.empty() && playerTurn < joinedPlayers.size()) {
			mutex.acquire();
			PlayerState& currentPlayerState = playerStates[playerTurn];

			// Quitting
			if (currentPlayerState == PlayerState::Quitting) {
				mutex.release();
				sendQuitMessage();
				if (cycleThroughPlayerTurns()) break;
				else continue;
			}

			// Skip not playing player
			if (currentPlayerState == PlayerState::NotPlaying) {
				mutex.release();
				sendPlayerSkippingTurn();
				if (cycleThroughPlayerTurns()) break;
				else continue;
			}

			// Check for blackjack
			if (playerHands[playerTurn].getTotalValue() == 21) {
				currentPlayerState = PlayerState::Blackjack;
				mutex.release();
				sendPlayersGotBlackjack();
				if (cycleThroughPlayerTurns()) break;
				else continue;
			}

			mutex.release();

			// Process player's turn
			while (currentPlayerState == PlayerState::TakingTurn) {
				sendDealerCardsInGame();
				sendPlayerState(playerTurn);

				sendPlayersTurnInstructions();

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
			std::string msg;

			if (hand.getTotalValue() > 21) {
				playerStates[i] = PlayerState::Bust;
				msg += "\n[BLACKJACK] " + joinedPlayers[i].getName() + " went bust...\n";
			}
			else if (hand.getTotalValue() == 21) {
				playerStates[i] = PlayerState::Blackjack;
				msg += "\n[BLACKJACK] " + joinedPlayers[i].getName() + " got blackjack!\n";
			}

			for (int i = 0; i < joinedPlayers.size(); i++) {
				if (playerStates[i] != PlayerState::Quitting) {
					m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
				}
			}
		}
		mutex.release();

		unlockFunction();
	}

	void stand(int i) {
		mutex.acquire();
		if (playerStates[i] != PlayerState::Quitting)
			playerStates[i] = PlayerState::DoneTakingTurn;
		
		std::string msg = "\n[BLACKJACK] " + joinedPlayers[playerTurn].getName() + " ends turn.\n";
		
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting) {
				m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), msg);
			}
		}
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

	void sendPlayerState(int i) {
		mutex.acquire();
		if (playerStates[i] == PlayerState::Quitting) {
			mutex.release();
			return;
		}

		const Player& player = joinedPlayers[i];
		const Hand& hand = playerHands[i];
		const int& bet = playerBets[i];

		std::string msg = "\n[BLACKJACK]\n";

		msg += "Player: " + player.getName() + '\n';
		msg += "Money: " + std::to_string(player.getMoney()) + '\n';
		msg += "Your bet: " + std::to_string(bet) + '\n';

		msg += "Your cards: ";
		for (int i = 0; i < hand.getCards().size() - 1; i++) {
			const Card& card = hand.getCards()[i];

			msg += std::to_string(card.getValue()) + ", ";
		}
		msg += std::to_string(hand.getCards()[hand.getCards().size() - 1].getValue()) + '\n';
		msg += "Total value: " + std::to_string(hand.getTotalValue()) + '\n';

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void sendDealerCardsInGame() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK]\nDealer's cards: " + std::to_string(dealerHand.getCards()[0].getValue());
		msg +=" ?\n";

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void printDealerCardsEndGame() {
		mutex.acquire();
		const auto& dealerCards = dealerHand.getCards();
		
		std::string msg = "\n[BLACKJACK]\n";
		msg += "Dealer's final cards: ";
		for (int i = 0; i < dealerCards.size() - 1; i++) {
			msg += std::to_string(dealerCards[i].getValue()) + ", ";
		}
		msg += std::to_string(dealerCards[dealerCards.size() - 1].getValue()) + '\n';
		msg += "Dealer's total value: " + std::to_string(dealerHand.getTotalValue()) + '\n';

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
		}
		mutex.release();
	}

	void printPlayerResults() {
		mutex.acquire();
		std::string msg = "\n[BLACKJACK]\n";
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] == PlayerState::Quitting) continue;
			const Player& player = joinedPlayers[i];
			const PlayerState& result = playerStates[i];

			msg += "Player: " + player.getName() + '\n';
			msg += "Money: " + std::to_string(player.getMoney()) + '\n';
			if (result == PlayerState::Win) {
				msg += "You win!\n";
			}
			else if (result == PlayerState::Push) {
				msg += "You pushed!\n";
			}
			else {
				msg += "You lost!\n";
			}
		}

		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting)
				playerMessages[i] += msg;
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

	void clearMessages() {
		mutex.acquire();
		for (int i = 0; i < playerMessages.size(); i++) {
			playerMessages[i] = " ";
		}
		mutex.release();
	}

	// callback function when server receives messages
	void Listener_MessageReceived(TcpListener* listener, int client, int sentClient, std::string msg) {
		// tries to find the command by splitting the string based on spaces
		std::string originalMsg = msg;
		std::string delimiter = " ";
		size_t pos = msg.find(delimiter);
		msg.erase(0, pos + delimiter.size());
		pos = msg.find(delimiter);
		std::string command = msg.substr(0, pos);

		if (command == "\\help") helpCommand(listener, client, sentClient);
		else if (command == "\\add") addCommand(client, sentClient, msg, pos, delimiter, originalMsg);
		else if (command == "\\bet") betCommand(client, sentClient, msg, pos, delimiter);
		else if (command == "\\hit" || command == "\\h") hitCommand(client, sentClient);
		else if (command == "\\stand" || command == "\\s") standCommand(client, sentClient);
		else if (command == "\\start") startCommand();
		else chat(listener, client, sentClient, originalMsg);
	}

	void helpCommand(TcpListener* listener, int client, int sentClient) {
		if (client != sentClient) return;

		mutex.acquire();
		std::string commandList = "\n[BLACKJACK]\n";
		commandList += "COMMAND LIST:\n";
		commandList += "\\start         - Starts game automatically.\n";
		commandList += "\\bet [integer] - Places integer amount of money for winning the round.\n";
		commandList += "\\hit           - Gets dealt another card when players turn (can also type \\h).\n";
		commandList += "\\stand         - Finish drawing cards and end player turn (can also type \\s).\n\n";
		commandList += "\nIf backslash is not indicated, inputted text turns to message\n that is sent to all other connected clients.\n";

		listener->sendMessageToClient(client, commandList);
		mutex.release();
	}

	void addCommand(int client, int sentClient, std::string msg, size_t pos, std::string delimiter, std::string originalMsg) {
		msg.erase(0, pos + delimiter.size());
		pos = msg.find(delimiter);

		mutex.acquire();
		// the first character of the message when user creates profile is always space
		// received messages from clients who are in the game do not start with space
		if (originalMsg[0] == ' ') {
			if (client == sentClient) {
				Player newPlayer(msg.substr(0, pos), client, 1000);
				mutex.release();

				// adds player to active players if max player size is not reached yet and 
				// the game has yet to start
				if (!gameStarted && joinedPlayers.size() < MAX_PLAYERS) addPlayer(newPlayer);
				else addPlayerToQueue(newPlayer);
			}
			else mutex.release();
		}
		else mutex.release();
	}

	void betCommand(int client, int sentClient, std::string msg, size_t pos, std::string delimiter) {
		mutex.acquire();
		if (gameStarted && sentClient == joinedPlayers[playerTurn].getClientNumber() &&
			sentClient == client && playerStates[playerTurn] == PlayerState::None) {

			// finds the number that the player betted
			msg.erase(0, pos + delimiter.size());
			pos = msg.find(delimiter);

			// converts number string into integer only if the first character in string starts with number
			std::string player_bet_string = msg.substr(0, pos);
			if ((int)player_bet_string[0] >= 48 && (int)player_bet_string[0] <= 57) {
				int player_bet = std::stoi(player_bet_string);

				if (player_bet <= 0) { // skips player turn if not playing
					playerStates[playerTurn] = PlayerState::NotPlaying;
				}
				else { // will mark player as playing this round if they have betted greater than 0
					playerBets[playerTurn] = player_bet;
					playerStates[playerTurn] = PlayerState::TakingTurn;
				}

				// tells game loop to progress
				unlockFunction();
			}
		}
		mutex.release();
	}

	void hitCommand(int client, int sentClient) {
		mutex.acquire();
		if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
			sentClient == client && playerStates[playerTurn] == PlayerState::TakingTurn) {
			mutex.release();

			hit(playerTurn);

			mutex.acquire();
		}
		mutex.release();
	}

	void standCommand(int client, int sentClient) {
		mutex.acquire();
		if (sentClient == joinedPlayers[playerTurn].getClientNumber() &&
			sentClient == client && playerStates[playerTurn] == PlayerState::TakingTurn) {
			mutex.release();

			stand(playerTurn);

			mutex.acquire();
		}
		mutex.release();
	}

	void startCommand() {
		mutex.acquire();
		if (!gameStarted)
			gameStarted = true;
		mutex.release();
	}

	void chat(TcpListener* listener, int client, int sentClient, std::string originalMsg) {
		if (client == sentClient) return;
		listener->sendMessageToClient(client, originalMsg);
	}

	void sendMessagesToClient() {
		mutex.acquire();
		// sends the string of accumulated meessages to the client
		for (int i = 0; i < joinedPlayers.size(); i++) {
			if (playerStates[i] != PlayerState::Quitting) {
				m_Listener->sendMessageToClient(joinedPlayers[i].getClientNumber(), playerMessages[i]);
			}
		}
		mutex.release();
	}

	bool cycleThroughPlayerTurns() {
		bool endPhase = false;
		mutex.acquire();
		playerTurn++;

		// goes back to the first player once it reaches end of player list
		if (playerTurn >= joinedPlayers.size()) {
			playerTurn = 0;
			endPhase = true;
		}
		mutex.release();

		return endPhase;
	}

	void lockFunction() {
		lock = true;
		
		sendMessagesToClient(); // sends messages to client before locking function
		
		while (lock); // prevents game loop functioning from proceeding until told to unlock

		clearMessages(); // resets messages with empty string
	}

	void unlockFunction() {
		lock = false;
	}

	// checks if there are any players still in the game
	bool verifyIfGameIsEmpty() {
		if (joinedPlayers.empty()) {
			if (queuedPlayersToJoin.empty())
				return true;

			unlockFunction();
		}

		return false;
	}

	const int MAX_PLAYERS = 6;

	TcpListener* m_Listener;
	std::vector<Card> deck;
	std::vector<Player> joinedPlayers;
	std::vector<Player> queuedPlayersToJoin;
	std::vector<PlayerState> playerStates;
	std::vector<Hand> playerHands;
	std::vector<int> playerBets;
	std::vector<int> clientsQuitting;
	std::vector<std::string> playerMessages;
	Hand dealerHand;
	int numPlayers;
	int playerTurn;
	bool lock;
	bool gameStarted;
	std::binary_semaphore mutex = std::binary_semaphore(1);
};

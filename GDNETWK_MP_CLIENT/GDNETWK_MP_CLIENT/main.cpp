#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <thread>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER_SIZE (10000)

std::string ipAddress = "127.0.0.1";		// IP Address of the server
int port = 54010;							// Listening port # on the server
std::string username = "";
std::string userInput = "";

SOCKET sock;

// prints out characters one at a time and saves it to global userInput string to
// prevent typed out messages from being cut off in the middle by received messages
// gotten from a different thread
void typeMessage() {
	while (true) {
		// waits for user to input a character
		char letter = _getch();
		
		// removes weird character such as arrow keys being printed
		if ((int)letter != 72 && (int)letter != 75 && (int)letter != 77 && (int)letter != 80 && (int)letter != -32) {
			if (letter == '\r') { // sends a message when inputted enter
				if (userInput.size() > 0) { // make sure the user has typed in something
					std::cout << std::endl << username << ": ";

					std::string message = username + ": " + userInput;

					// send text to listening socket
					int sendResult = send(sock, message.c_str(), message.size() + 1, 0);

					// empties out userInput string
					userInput = "";
				}
			}
			else if (letter == '\b') { // deletes last character in inputted string when inputting backspace
				if (!userInput.empty()) {
					std::cout << "\b \b";
					userInput.pop_back();
				}
			}
			else { // otherwise, prints out the character and adds it to input string
				std::cout << letter;
				userInput.push_back(letter);
			}
		}
	}
}

void receiveMessage() {
	while (true) {
		char buf[MAX_BUFFER_SIZE];

		// wait for response
		ZeroMemory(buf, MAX_BUFFER_SIZE);
		int bytesReceived = recv(sock, buf, MAX_BUFFER_SIZE, 0);
		if (bytesReceived > 0) { // echo response to console if bytes were received
			int inputSize = userInput.size();

			for (int i = 0; i < inputSize + username.size() + 2; i++) {
				std::cout << "\b \b";
			}

			std::cout << std::string(buf, 0, bytesReceived) << std::endl << username << ": " << userInput;
		}
	}
}

void createGameProfile() {
	std::cout << "Enter your name: ";
	std::getline(std::cin, username);
	for (int i = 0; i < username.size(); i++) {
		if (username[i] == ' ')
			username[i] = '_';
	}
	std::cout << std::endl << "[BLACKJACK]\nAwesome " << username << "! Welcome to BLACKJACK." << std::endl;
	std::cout << "Type \\help for list of available commands to play the game." << std::endl;

	// Adds user to game
	std::string message = " \\add " + username;
	send(sock, message.c_str(), message.size() + 1, 0);
}

void cleanup() {
	WSACleanup();
}

bool createSocket() {
	// create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, Error #" << WSAGetLastError() << ". Quitting..." << std::endl;
		cleanup();
		return false;
	}

	// fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << ". Quitting..." << std::endl;
		closesocket(sock);
		WSACleanup();
		return false;
	}

	return true;
}

bool init() {
	// initialize winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start Winsock, Error #" << wsResult << ". Quitting..." << std::endl;
		return false;
	}

	return createSocket();
 }

int main() {
	// initialize winsock
	if (!init()) return -1;

	createGameProfile();

	// runs byte receiving in separate thread
	std::thread(&receiveMessage).detach();

	// handles byte sending in main thread
	typeMessage();
	
	// gracefully close down everything
	closesocket(sock);
	WSACleanup();

	return 0;
}
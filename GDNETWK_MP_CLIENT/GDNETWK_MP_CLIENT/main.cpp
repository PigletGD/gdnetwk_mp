#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <thread>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

std::string ipAddress = "127.0.0.1";		// IP Address of the server
int port = 54010;							// Listening port # on the server
std::string username = "";
std::string userInput = "";

SOCKET sock;

// prints out characters one at a time and saves it to global userInput string to
// prevent typed out messages from being cut off in the middle by received messages
void typeMessage() {
	do {
		char letter = _getch();

		if (letter == '\r') {
			if (userInput.size() > 0) { // make sure the user has typed in something
				std::cout << std::endl << std::endl;

				std::string message = username + ": " + userInput;

				// send the text
				int sendResult = send(sock, message.c_str(), message.size() + 1, 0);

				userInput = "";
				std::cout << username << ": ";
			}
		}
		else if (letter == '\b') {
			if (!userInput.empty()) {
				std::cout << "\b \b";

				userInput.pop_back();
			}
		}
		else {
			std::cout << letter;

			userInput.push_back(letter);
		}
	} while (true);
}

void receiveMessage() {
	while (true) {
		char buf[4096];

		// wait for response
		ZeroMemory(buf, 4096);
		int bytesReceived = recv(sock, buf, 4096, 0);
		if (bytesReceived > 0) { // echo response to console if bytes were received
			int inputSize = userInput.size();

			for (int i = 0; i < inputSize; i++) {
				std::cout << "\b \b";
			}

			std::cout << std::string(buf, 0, bytesReceived) << std::endl << username << ": " << userInput;
		}
	}
}

int main() {
	// initialize winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start Winsock, Error #" << wsResult << ". Quitting..." << std::endl;
		return 69;
	}

	// create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, Error #" << WSAGetLastError() << ". Quitting..." << std::endl;
		WSACleanup();
		return 420;
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
		return -1;
	}

	std::cout << "Type in username: ";
	std::getline(std::cin, username);
	std::cout << std::endl;
	
	std::thread(&typeMessage).detach();
	std::thread(&receiveMessage).detach();

	while (true);

	// gracefully close down everything
	closesocket(sock);
	WSACleanup();

	return 0;
}
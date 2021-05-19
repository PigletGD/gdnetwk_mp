#include <iostream>
#include <string>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

std::string ipAddress = "127.0.0.1";		// IP Address of the server
int port = 54010;							// Listening port # on the server

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
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
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
	
	// do-while loop to send and receive data
	char buf[4096];
	std::string userInput;

	do {
		// prompt user for some text
		std::cout << "> ";
		std::getline(std::cin, userInput);

		if (userInput.size() > 0) { // make sure the user has typed in something
			// send the text
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);

			// wait for response
			ZeroMemory(buf, 4096);
			int bytesReceived = recv(sock, buf, 4096, 0);
			if (bytesReceived > 0) { // echo response to console if bytes were received
				std::cout << "SERVER> " << std::string(buf, 0, bytesReceived) << std::endl;
			}
		}
	} while (userInput.size() > 0);

	// gracefully close down everything
	closesocket(sock);
	WSACleanup();

	return 0;
}
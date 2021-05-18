#include <iostream>
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

int main() {

	// initialize winsock
	WSADATA wsData; // variable to hold data when starting up winsock
	WORD ver = MAKEWORD(2, 2); // indicates version of winsock (ver. 2.2)

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		std::cerr << "Can't initialize winsock! Quitting..." << std::endl;
		return -1;
	}

	// create a socket
	// AF (Address Family); SOCK_STREAM (TCP Socket)
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		std::cerr << "Can't create a socket! Quitting..." << std::endl;
		return 420;
	}

	// bind socket to ip address and port
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // could also use inet_pton ....

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// tell winsock the socket is for listening
	listen(listening, SOMAXCONN);

	// wait for connection
	sockaddr_in client;
	int clientSize = sizeof(client);

	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Can't accept client! Quitting for now (but change this later)..." << std::endl;
		// do something for error handling other than exiting program
		return 69;
	}

	char host[NI_MAXHOST];		// client's remote name
	char service[NI_MAXHOST];	// SERVICE (ie. port) client is connected on

	ZeroMemory(host, NI_MAXHOST);		// same as memset(host, 0, NI_MAXHOST);
	ZeroMemory(service, NI_MAXHOST);

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		std::cout << host << " connected on port " << service << std::endl;
	}
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << "connected on port " << ntohs(client.sin_port) << std::endl;
	}

	// close listening socket
	closesocket(listening); // temporary, cuz will implement closing sockets later

	// while loop: accept and echo message back to client
	char buf[4096];

	while (true) {
		ZeroMemory(buf, 4096);

		// wait for client to send data
		int bytesReceived = recv(clientSocket, buf, 4096, 0);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "Error in recv(). Quitting..." << std::endl;
			break;
		}

		if (bytesReceived == 0) {
			std::cout << "Client disconnected..." << std::endl;
			break;
		}

		// Echo message back to client
		send(clientSocket, buf, bytesReceived + 1, 0);
	}

	// close the socket
	closesocket(clientSocket);

	// clean up winsock
	WSACleanup();

	return 0;
}
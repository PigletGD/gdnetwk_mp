#include <iostream>
#include <string>
#include <sstream>

#include "TcpListener.h"

std::string ipAddress = "127.0.0.1";
int port = 54010;

void Listener_MessageReceived(TcpListener* listener, int client, std::string msg);

int oldmain() {
	TcpListener server(ipAddress, port, Listener_MessageReceived);

	if (server.init())
		server.run();

	return 0;
}

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
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // could also use inet_pton ....

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// tell winsock the socket is for listening
	listen(listening, SOMAXCONN);

	fd_set master;
	FD_ZERO(&master);
	FD_SET(listening, &master);

	while (true) {
		fd_set copy = master;

		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++) {
			SOCKET socket = copy.fd_array[i];
			if (socket == listening) {
				// accept a new connection
				SOCKET client = accept(listening, nullptr, nullptr);

				// add the new connection to the list of connected clients
				FD_SET(client, &master);

				// send a welcome message to the connected client
				std::string welcomeMsg = "Welcome to the Awesome Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);

				// todo: broadcast we have a new connection
			}
			else {
				char buf[4096];
				ZeroMemory(buf, 4096);

				int bytesIn = recv(socket, buf, 4096, 0);
				if (bytesIn <= 0) {
					// drop client
					closesocket(socket);
					FD_CLR(socket, &master);
				}
				else {
					// send message to other clients, and definitely not listening socket
					for (int i = 0; i < master.fd_count; i++) {
						SOCKET outSock = master.fd_array[i];
						if (outSock != listening && outSock != socket) {
							std::ostringstream ss;
							ss << "SOCKET #" << socket << ": " << buf << "\r\n";
							std::string strOut = ss.str();
							send(outSock, strOut.c_str(), strOut.size() + 1, 0);
						}
					}
				}
			}
		}
	}

	return 0;
}

void Listener_MessageReceived(TcpListener* listener, int client, std::string msg) {
	listener->sendMessageToClient(client, msg);
}
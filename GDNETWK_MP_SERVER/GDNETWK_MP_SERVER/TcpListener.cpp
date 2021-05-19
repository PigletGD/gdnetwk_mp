#include "TcpListener.h"

TcpListener::TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler)
	: m_ipAddress(ipAddress), m_port(port), MessageReceived(handler) {

}

TcpListener::~TcpListener() {
	cleanup();
}

void TcpListener::sendMessageToClient(int clientSocket, std::string msg) {
	send(clientSocket, msg.c_str(), msg.size() + 1, 0);
}

// initialize winsock
bool TcpListener::init() {
	WSADATA wsData; // variable to hold data when starting up winsock
	WORD ver = MAKEWORD(2, 2); // indicates version of winsock (ver. 2.2)

	int wsInit = WSAStartup(ver, &wsData);
	
	// TODO: Inform caller the error that occured
	
	/*if (wsInit != 0) {
		std::cerr << "Can't initialize winsock! Quitting..." << std::endl;
		return -1;
	}*/

	return wsInit == 0;
}

// main processing loop
void TcpListener::run() {
	char buf[MAX_BUFFER_SIZE];

	while (true) {
		SOCKET listening = createSocket();
		if (listening == INVALID_SOCKET) {
			break;
		}

		SOCKET client = waitForConnection(listening);
		if (client != INVALID_SOCKET) {
			closesocket(listening);
			int bytesReceived = 0;
			do {
				ZeroMemory(buf, MAX_BUFFER_SIZE);

				// wait for client to send data
				bytesReceived = recv(client, buf, MAX_BUFFER_SIZE, 0);
				if (bytesReceived > 0) {
					if (MessageReceived != NULL) {
						std::cout << std::string(buf, 0, bytesReceived) << std::endl;

						MessageReceived(this, client, std::string(buf, 0, bytesReceived));
					}
				}

				//std::cout << std::string(buf, 0, bytesReceived) << std::endl;

				// echo message back to client
				//send(client, buf, bytesReceived + 1, 0);
			} while (bytesReceived > 0);

			closesocket(client);
		}		
	}
}

// clean up afer using the service
void TcpListener::cleanup() {
	WSACleanup();
}

// create a socket
SOCKET TcpListener::createSocket() {
	// AF (Address Family); SOCK_STREAM (TCP Socket)
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening != INVALID_SOCKET) {
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(m_port);
		inet_pton(AF_INET, m_ipAddress.c_str(), &hint.sin_addr);

		int bindOk = bind(listening, (sockaddr*)&hint, sizeof(hint));
		if (bindOk != SOCKET_ERROR) {
			int listenOk = listen(listening, SOMAXCONN);
			if (listenOk == SOCKET_ERROR) return -1;
		}
		else return -1;
	}
	
	return listening;
}

// wait for a connection
SOCKET TcpListener::waitForConnection(SOCKET listening)
{
	SOCKET client = accept(listening, NULL, NULL);

	return client;
}

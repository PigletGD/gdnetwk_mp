#include "TcpListener.h"
#include "Game.h"

TcpListener::TcpListener(std::string ipAddress, int port)
	: m_ipAddress(ipAddress), m_port(port) {

}

TcpListener::~TcpListener() {
	cleanup();
}

void TcpListener::setGame(Game* game) {
	m_Game = game;
}

void TcpListener::setCallback(MessageReceivedHandler handler) {
	MessageReceived = handler;
}

void TcpListener::sendMessageToClient(int clientSocket, std::string msg) {
	send(clientSocket, msg.c_str(), msg.size() + 1, 0);
}

// initialize winsock
bool TcpListener::init() {
	// initialize winsock
	WSADATA wsData; // variable to hold data when starting up winsock
	WORD ver = MAKEWORD(2, 2); // indicates version of winsock (ver. 2.2)
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		std::cerr << "Can't initialize winsock! Quitting..." << std::endl;
		return false;
	}

	createSocket();

	return listening != -1;
}

// main processing loop
void TcpListener::run() {
	char buf[MAX_BUFFER_SIZE];

	FD_ZERO(&master);
	FD_SET(listening, &master);

	while (true) {
		fd_set copy = master;

		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++) {
			SOCKET socket = copy.fd_array[i];
			if (socket == listening) listenForConnections();
			else checkConnectedSocket(socket, buf);
		}
	}
}

// clean up afer using the service
void TcpListener::cleanup() {
	WSACleanup();
}

// create a socket
void TcpListener::createSocket() {
	// AF (Address Family); SOCK_STREAM (TCP Socket)
	listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		std::cerr << "Can't create a socket! Quitting..." << std::endl;
		cleanup();
		return;
	}

	// bind socket to ip address and port
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // could also use inet_pton ....
	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// tell winsock the socket is for listening
	int listenOk = listen(listening, SOMAXCONN);
	if (listenOk == SOCKET_ERROR) return;
}

void TcpListener::listenForConnections() {
	SOCKET client = waitForConnection();
	// add the new connection to the list of connected clients
	FD_SET(client, &master);

	// ensures client in waiting gets sent blanket message
	sendMessageToClient(client, " \b");
}

// wait for a connection
SOCKET TcpListener::waitForConnection()
{
	SOCKET client = accept(listening, nullptr, nullptr);

	return client;
}

void TcpListener::checkConnectedSocket(SOCKET socket, char* buf) {
	ZeroMemory(buf, MAX_BUFFER_SIZE);

	int bytesIn = recv(socket, buf, MAX_BUFFER_SIZE, 0);
	if (bytesIn <= 0) {
		// drop client
		closesocket(socket);
		m_Game->queuePlayerToRemove(socket);
		FD_CLR(socket, &master);
	}
	else {
		// send message to other clients, and definitely not listening socket
		for (int i = 0; i < master.fd_count; i++) {
			SOCKET outSock = master.fd_array[i];
			if (MessageReceived != NULL && outSock != listening) {
				std::ostringstream ss;
				ss << buf << " \r" << std::endl;
				std::string strOut = ss.str();
				MessageReceived(this, outSock, socket, std::string(buf, 0, bytesIn));
			}
		}
	}
}

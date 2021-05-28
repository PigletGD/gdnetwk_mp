#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include <WS2tcpip.h>					// header file for winsock functions
#pragma comment (lib, "ws2_32.lib")		// winsock library file

#define MAX_BUFFER_SIZE (41912)

// forward declaration of class
class TcpListener;
class Game;

// callback to data received
typedef void (*MessageReceivedHandler)(TcpListener* listener, int socketId, std::string msg);

class TcpListener {
public:
	TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler);

	~TcpListener();

	void setGame(Game* game);

	// send message to specified client
	void sendMessageToClient(int clientSocket, std::string msg);

	// initialize winsock
	bool init();

	// main processing loop
	void run();

	// clean up afer using the service
	void cleanup();
private:
	// create a socket
	void createSocket();

	// wait for a connection
	SOCKET waitForConnection();

	Game* m_Game;
	SOCKET					listening;
	fd_set					master;
	std::string				m_ipAddress;
	int						m_port;
	MessageReceivedHandler	MessageReceived;
};
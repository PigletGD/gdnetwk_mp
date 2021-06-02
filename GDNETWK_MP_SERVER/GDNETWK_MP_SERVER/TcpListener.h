#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <functional>

#include <WS2tcpip.h>					// header file for winsock functions
#pragma comment (lib, "ws2_32.lib")		// winsock library file

#define MAX_BUFFER_SIZE (10000)

// forward declaration of class
class TcpListener;
class Game;

// callback to data received
typedef std::function<void(TcpListener* listener, int socketId, int sentSocketId, std::string msg)> MessageReceivedHandler;

class TcpListener {
public:
	TcpListener(std::string ipAddress, int port);

	~TcpListener();

	void setGame(Game* game);

	void setCallback(MessageReceivedHandler handler);

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

	// listens for connections to connect
	void listenForConnections();

	// wait for a connection
	SOCKET waitForConnection();

	void checkConnectedSocket(SOCKET socket, char* buf);

	Game* m_Game;
	SOCKET					listening;
	fd_set					master;
	std::string				m_ipAddress;
	int						m_port;
	MessageReceivedHandler	MessageReceived;
};
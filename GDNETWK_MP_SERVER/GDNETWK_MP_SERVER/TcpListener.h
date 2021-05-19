#pragma once

#include <iostream>
#include <string>

#include <WS2tcpip.h>					// header file for winsock functions
#pragma comment (lib, "ws2_32.lib")		// winsock library file

#define MAX_BUFFER_SIZE (41912)

// forward declaration of class
class TcpListener;

// callback to data received
typedef void (*MessageReceivedHandler)(TcpListener* listener, int socketId, std::string msg);

class TcpListener {
public:
	TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler);

	~TcpListener();

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
	SOCKET createSocket();

	// wait for a connection
	SOCKET waitForConnection(SOCKET listening);

	std::string				m_ipAddress;
	int						m_port;
	MessageReceivedHandler	MessageReceived;
};
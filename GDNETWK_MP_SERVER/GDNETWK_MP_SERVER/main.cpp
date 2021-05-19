#include <iostream>
#include <string>

#include "TcpListener.h"

std::string ipAddress = "127.0.0.1";
int port = 54010;

void Listener_MessageReceived(TcpListener* listener, int client, std::string msg);

int main() {
	TcpListener server(ipAddress, port, Listener_MessageReceived);

	if (server.init())
		server.run();

	return 0;
}

void Listener_MessageReceived(TcpListener* listener, int client, std::string msg) {
	listener->sendMessageToClient(client, msg);
}
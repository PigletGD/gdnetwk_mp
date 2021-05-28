#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include "Game.h"

std::string ipAddress = "127.0.0.1";
int port = 54010;
SOCKET listening;

int main() {
	TcpListener server(ipAddress, port);
	Game game(&server);

	server.setGame(&game);
	std::thread gameThread(&Game::run, &game);

	if (server.init())
		server.run();

	return 0;
}
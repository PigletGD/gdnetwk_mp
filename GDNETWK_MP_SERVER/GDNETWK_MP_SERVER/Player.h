#pragma once
#include <string>

class Player {
public:
	Player(const std::string& name, int clientNumber, int money) : name(name), clientNumber(clientNumber), money(money) { }

	void setMoney(int value) {
		money = value;
	}

	std::string getName() const {
		return name;
	}

	int getMoney() const {
		return money;
	}

	int getClientNumber() const {
		return clientNumber;
	}

private:
	std::string name;
	int money;
	int clientNumber;
};

#pragma once
#include <string>

class Player {
public:
	Player(const std::string& name, int money) : name(name), money(money) { }

	void setMoney(int value) {
		money = value;
	}

	std::string getName() const {
		return name;
	}

	int getMoney() const {
		return money;
	}

private:
	std::string name;
	int money;
};

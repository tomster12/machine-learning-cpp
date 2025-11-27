#pragma once

#include "AppScenarioController.h"
#include "UIManager.h"

class App
{
public:
	virtual ~App();
	void run();

protected:
	std::unique_ptr<AppScenarioController> controller;
	sf::RenderWindow* window = nullptr;
	sf::Clock dtClock;
	float dt = 0;

private:
	void initialize();
	void update();
	void render();
};

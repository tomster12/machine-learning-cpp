#pragma once

#include "AppScenarioController.h"
#include "UIManager.h"

class App
{
public:
	virtual ~App();
	int run();

protected:
	virtual void setupUI();

	std::unique_ptr<AppScenarioController> controller;
	std::unique_ptr<UIManager> ui;
	sf::RenderWindow* window = nullptr;
	sf::Clock dtClock;
	float dt = 0;

private:
	int initialize();
	void update();
	void render();
};

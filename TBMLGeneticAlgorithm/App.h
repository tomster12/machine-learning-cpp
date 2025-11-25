#pragma once

#include "GenepoolController.h"
#include "UIManager.h"

class App
{
public:
	virtual ~App();
	int run();

protected:
	virtual void setupUI();
	virtual tbml::ga::IGenepoolPtr createGenepool() = 0;

	std::unique_ptr<GenepoolController> controller;
	std::unique_ptr<UIManager> ui;
	sf::RenderWindow* window = nullptr;
	sf::Clock dtClock;
	float dt = 0;

private:
	void initialize();
	void update();
	void render();
};

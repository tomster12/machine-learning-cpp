#pragma once

#include "AppGenepoolController.h"
#include "UIManager.h"

class App
{
public:
	virtual ~App();
	int run(IAppGenepoolPtr&& genepool);

protected:
	virtual void setupUI();

	std::unique_ptr<AppGenepoolController> controller;
	std::unique_ptr<UIManager> ui;
	sf::RenderWindow* window = nullptr;
	sf::Clock dtClock;
	float dt = 0;

private:
	int initialize(IAppGenepoolPtr&& genepool);
	void update();
	void render();
};

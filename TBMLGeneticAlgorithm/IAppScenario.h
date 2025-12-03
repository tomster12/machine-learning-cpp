#pragma once
#include <GenepoolSimulation.h>
#include "UIManager.h"

class IAppScenario
{
public:
	virtual void initUI(sf::RenderWindow* window, UIManager* ui) {};
	virtual void update() {};
	virtual void render(sf::RenderWindow* window) {};
	virtual tbml::ga::IGenepoolPtr getGenepool() const = 0;
};

using IAppScenarioUPtr = std::unique_ptr<IAppScenario>;

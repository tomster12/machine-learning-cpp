#pragma once
#include <GenepoolSimulation.h>

class IAppScenario
{
public:
	virtual void render(sf::RenderWindow* window) = 0;
	virtual tbml::ga::IGenepoolPtr getGenepool() const = 0;
};

using IAppScenarioUPtr = std::unique_ptr<IAppScenario>;

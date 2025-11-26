#pragma once

#include "stdafx.h"
#include "GenepoolSimulation.h"

class IAppScenario
{
public:
	virtual void render(sf::RenderWindow* window) = 0;
	virtual tbml::ga::IGenepoolPtr getGenepool() const = 0;
};

using IAppScenarioUPtr = std::unique_ptr<IAppScenario>;

class AppScenarioController
{
public:
	AppScenarioController() = default;
	AppScenarioController(IAppScenarioUPtr&& scenario);

	void update();
	void render(sf::RenderWindow* window);
	void iterateGeneration();

	void setEvaluate(bool v) { toEvaluate = v; }
	void setFullEvaluate(bool v) { toFullEvaluate = v; }
	void setAutoIterate(bool v) { toAutoIterate = v; }
	void setShowVisuals(bool v) { genepool->setShowVisuals(v); }
	tbml::ga::IGenepoolPtr getGenepool() { return genepool; }

private:
	IAppScenarioUPtr scenario = nullptr;
	tbml::ga::IGenepoolPtr genepool = nullptr;
	bool toEvaluate = false;
	bool toFullEvaluate = false;
	bool toAutoIterate = false;
};

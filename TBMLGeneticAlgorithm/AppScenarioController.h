#pragma once

#include "stdafx.h"
#include "GenepoolSimulation.h"
#include "UIManager.h"
#include "IAppScenario.h"

class AppScenarioController
{
public:
	AppScenarioController() = default;
	AppScenarioController(IAppScenarioUPtr&& scenario, sf::RenderWindow* window);

	void initUI(sf::RenderWindow* window);
	void update();
	void render(sf::RenderWindow* window);
	void iterateGeneration();
	void setEvaluate(bool v) { toEvaluate = v; }
	void setFullEvaluate(bool v) { toFullEvaluate = v; }
	void setAutoIterate(bool v) { toAutoIterate = v; }
	void setShowVisuals(bool v) { toShowVisuals = v; }
	tbml::ga::IGenepoolPtr getGenepool() { return genepool; }

private:
	std::unique_ptr<UIManager> ui = nullptr;
	IAppScenarioUPtr scenario = nullptr;
	tbml::ga::IGenepoolPtr genepool = nullptr;
	bool toShowVisuals = false;
	bool toEvaluate = false;
	bool toFullEvaluate = false;
	bool toAutoIterate = false;
};

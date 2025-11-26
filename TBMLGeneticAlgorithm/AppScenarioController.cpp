#include "stdafx.h"
#include "AppScenarioController.h"

AppScenarioController::AppScenarioController(IAppScenarioUPtr&& scenario)
	: scenario(std::move(scenario))
{
	this->genepool = scenario->getGenepool();
}

void AppScenarioController::update()
{
	if (!genepool->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");
	if (!genepool->getGenerationEvaluated() && toEvaluate) genepool->evaluateGeneration(!toFullEvaluate);
	if (genepool->getGenerationEvaluated() && toAutoIterate) genepool->iterateGeneration();
}

void AppScenarioController::render(sf::RenderWindow* window)
{
	if (!genepool->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");
	scenario->render(window);
}

void AppScenarioController::iterateGeneration()
{
	if (!genepool->getGenepoolInitialized()) return;
	if (!genepool->getGenerationEvaluated()) return;
	genepool->iterateGeneration();
}

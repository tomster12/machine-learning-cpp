#include "stdafx.h"
#include "AppGenepoolController.h"

AppGenepoolController::AppGenepoolController(IAppGenepoolPtr&& pool)
	: controller(std::move(pool))
{}

void AppGenepoolController::update()
{
	if (!controller->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");
	if (!controller->getGenerationEvaluated() && toEvaluate) controller->evaluateGeneration(!toFullEvaluate);
	if (controller->getGenerationEvaluated() && toAutoIterate) controller->iterateGeneration();
}

void AppGenepoolController::render(sf::RenderWindow* window)
{
	if (!controller->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");
	controller->render(window);
}

void AppGenepoolController::iterateGeneration()
{
	if (!controller->getGenepoolInitialized()) return;
	if (!controller->getGenerationEvaluated()) return;
	controller->iterateGeneration();
}

void AppGenepoolController::setEvaluate(bool v) { toEvaluate = v; }

void AppGenepoolController::setFullEvaluate(bool v) { toFullEvaluate = v; }

void AppGenepoolController::setAutoIterate(bool v) { toAutoIterate = v; }

void AppGenepoolController::setShowVisuals(bool v) { controller->setShowVisuals(v); }

IAppGenepoolPtr AppGenepoolController::getGenepool() const { return controller; }

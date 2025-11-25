#include "stdafx.h"
#include "GenepoolController.h"

GenepoolController::GenepoolController(tbml::ga::IGenepoolPtr&& pool)
	: genepool(std::move(pool))
{}

void GenepoolController::update()
{
	if (!genepool->getGenepoolInitialized())
	{
		throw std::runtime_error("Genepool not initialized.");
	}
	if (!genepool->getGenerationEvaluated())
	{
		if (toEvaluate) genepool->evaluateGeneration(!toFullEvaluate);
	}
	if (genepool->getGenerationEvaluated())
	{
		if (toAutoIterate) genepool->iterateGeneration();
	}
}

void GenepoolController::render(sf::RenderWindow* window)
{
	if (!genepool->getGenepoolInitialized())
	{
		throw std::runtime_error("Genepool not initialized.");
	}
	genepool->render(window);
}

void GenepoolController::iterateGeneration()
{
	if (!genepool->getGenepoolInitialized()) return;
	if (!genepool->getGenerationEvaluated()) return;
	genepool->iterateGeneration();
}

void GenepoolController::setEvaluate(bool v) { toEvaluate = v; }
void GenepoolController::setFullEvaluate(bool v) { toFullEvaluate = v; }
void GenepoolController::setAutoIterate(bool v) { toAutoIterate = v; }
void GenepoolController::setShowVisuals(bool v) { genepool->setShowVisuals(v); }

tbml::ga::IGenepoolPtr GenepoolController::getGenepool() const { return genepool; }

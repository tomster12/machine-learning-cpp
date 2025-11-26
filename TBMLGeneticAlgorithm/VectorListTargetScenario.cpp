#include "stdafx.h"
#include "VectorListTargetGenepool.h"
#include "VectorListTargetScenario.h"

VectorListTargetScenario::VectorListTargetScenario()
{
	this->genepool = std::make_shared<VectorListTargetGenepool>(
		[]() { return std::make_shared<VectorListGenome>(500); },
		nullptr,
		sf::Vector2f{ 700.0f, 100.0f },
		20.0f
	);

	this->genepool->setCreateAgentFn([=](VectorListTargetGenepool::GenomeCPtr data)
	{
		return std::make_unique<VectorListTargetAgent>(
			std::move(data),
			genepool,
			sf::Vector2f{ 700.0f, 600.0f },
			4.0f,
			4.0f
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(1000, 0.04f);
}

void VectorListTargetScenario::render(sf::RenderWindow* window)
{
	this->genepool->render(window);
}
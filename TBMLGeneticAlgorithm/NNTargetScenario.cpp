#include "stdafx.h"
#include "NNTargetScenario.h"
#include "Body.h"

NNTargetScenario::NNTargetScenario()
	: nnRenderer({ { "target x", 500.0f }, { "target y", 500.0f }, { "speed x", 500.0f }, { "speed y", 500.0f } }, { "x acc", "y acc" })
{
	this->genepool = std::make_shared<NNTargetGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(4, 2),
			std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr, std::vector<sf::Vector2f> { {300, 150}, { 1100,400 }, { 450,850 }, { 700,320 } }, 4.0f
	);

	this->genepool->setCreateAgentFn([=](NNTargetGenepool::GenomeCPtr data)
	{
		return std::make_unique<NNTargetAgent>(
			std::move(data), this->genepool.get(),
			sf::Vector2f{ 700, 850 }, 2.0f, 400.0f, 0.99f, 3000
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(1000, 0.1f);
}

void NNTargetScenario::renderWorld(sf::RenderWindow* window)
{
	this->genepool->renderWorld(window);
}

void NNTargetScenario::renderAgents(sf::RenderWindow* window)
{
	this->renderGenepoolAgents(window, *genepool->getAgentPopulation());
}

void NNTargetScenario::renderOverlay(sf::RenderWindow* window)
{
	const auto& elite = getElite(*genepool->getAgentPopulation());
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

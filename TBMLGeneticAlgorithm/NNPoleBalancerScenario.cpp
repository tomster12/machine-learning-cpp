#include "stdafx.h"
#include "CommonImpl.h"
#include "NNPoleBalancerScenario.h"

NNPoleBalancerScenario::NNPoleBalancerScenario()
{
	this->genepool = std::make_shared<tbml::ga::Genepool<NNGenome, NNPoleBalancerAgent>>(
		[]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(4, 1),
			std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		[](std::shared_ptr<const NNGenome> genome)
	{
		return std::make_unique<NNPoleBalancerAgent>(
			std::move(genome),
			1.0f, 0.1f, 1.0f, 1.0f, 2.5f, 4.0f, 30.0f
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(1000, 0.1f);
}

void NNPoleBalancerScenario::render(sf::RenderWindow* window)
{
	if (!this->genepool->getGenepoolInitialized()) throw std::runtime_error("tbml::GenepoolSimulation: Cannot render because uninitialized.");

	for (const auto& inst : *this->genepool->getAgentPopulation()) inst->render(window);
}

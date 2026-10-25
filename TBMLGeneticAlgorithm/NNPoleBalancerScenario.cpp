#include "stdafx.h"
#include "CommonImpl.h"
#include "NNPoleBalancerScenario.h"

namespace
{
	constexpr float TRACK_LIMIT = 1.0f;
	constexpr float ANGLE_LIMIT = 0.21f;
}

NNPoleBalancerScenario::NNPoleBalancerScenario()
	: nnRenderer({ { "position", TRACK_LIMIT }, { "velocity", 2.0f }, { "angle", ANGLE_LIMIT }, { "ang. velocity", 1.0f } }, { "force" })
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
			1.0f, 0.1f, 1.0f, 1.0f, TRACK_LIMIT, ANGLE_LIMIT, 30.0f
		);
	});

	// Only one neuron to recombine, so adapting the mutation rate is what keeps it from stalling
	this->genepool->setAdaptiveMutation(true);
	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(1000, 0.1f);
}

void NNPoleBalancerScenario::renderAgents(sf::RenderWindow* window)
{
	this->renderGenepoolAgents(window, *genepool->getAgentPopulation());
}

void NNPoleBalancerScenario::renderOverlay(sf::RenderWindow* window)
{
	const auto& elite = getElite(*genepool->getAgentPopulation());
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

#include "stdafx.h"
#include "NNTargetScenario.h"
#include "Body.h"

NNTargetScenario::NNTargetScenario()
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

void NNTargetScenario::render(sf::RenderWindow* window)
{
	this->genepool->render(window);
}

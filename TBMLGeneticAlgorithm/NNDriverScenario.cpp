#include "stdafx.h"
#include "TbmlGlobal.h"
#include "CommonImpl.h"
#include "NNDriverScenario.h"
#include "Body.h"
#include "UIButton.h"

NNDriverScenario::NNDriverScenario()
{
	float PI = 3.14159265359f;

	std::vector<Body> worldBodies;
	worldBodies.push_back(Body({ 150.0f, 550.0f }, { 50.0f, 850.0f }, PI * 0.01f));
	worldBodies.push_back(Body({ 520.0f, 620.0f }, { 450.0f, 50.0f }, PI * 0.4f));
	worldBodies.push_back(Body({ 640.0f, 150.0f }, { 1000.0f, 50.0f }, 0));
	worldBodies.push_back(Body({ 600.0f, 400.0f }, { 400.0f, 50.0f }, 0));
	worldBodies.push_back(Body({ 800.0f, 600.0f }, { 400.0f, 50.0f }, PI * 0.45f));
	worldBodies.push_back(Body({ 1200.0f, 480.0f }, { 700.0f, 50.0f }, PI * 0.4f));

	std::vector<sf::Vector2f> targets;
	targets.push_back({ 250.0f, 270.0f });
	targets.push_back({ 1000.0f, 280.0f });
	targets.push_back({ 1200.0f, 750.0f });
	targets.push_back({ 550.0f, 930.0f });

	targets.push_back({ 350.0f, 370.0f });
	targets.push_back({ 900.0f, 320.0f });
	targets.push_back({ 950.0f, 700.0f });
	targets.push_back({ 550.0f, 930.0f });

	this->genepool = std::make_shared<NNDriverGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
				std::make_shared<tbml::nn::Layer::Dense>(8, 5),
				std::make_shared<tbml::nn::Layer::ReLU>(),
				std::make_shared<tbml::nn::Layer::Dense>(5, 5),
				std::make_shared<tbml::nn::Layer::ReLU>(),
				std::make_shared<tbml::nn::Layer::Dense>(5, 2),
				std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr,
		targets,
		20.0f,
		worldBodies
	);

	this->genepool->setCreateAgentFn(
		[=](NNDriverGenepool::GenomeCPtr data)
	{
		return std::make_unique<NNDriverAgent>(
			std::move(data), this->genepool.get(),
			sf::Vector2f{ 300.0f, 780.0f }, 1000.0f, 25.0f, 0.5f, 0.95f, 120.0f, 0.25f, 300
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(2000, 0.05f);
	this->genepool->logInformation();
}

void NNDriverScenario::render(sf::RenderWindow* window)
{
	this->genepool->render(window);

	//NNDriverGenepool::AgentPtr bestAgent = this->genepool->getBestAgent();
	//if (bestAgent != nullptr)
	//{
	//	const tbml::nn::NeuralNetwork& bestNetwork = bestAgent->getGenome()->getNetwork();
	//	this->nnRenderer.render(window, bestNetwork);
	//}
}

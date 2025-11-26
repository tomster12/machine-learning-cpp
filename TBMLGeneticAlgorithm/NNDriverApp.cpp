#include "stdafx.h"
#include "NNDriverApp.h"
#include "NNDriverGenepool.h"
#include "CommonImpl.h"
#include "GenepoolSimulation.h"
#include "Body.h"

tbml::ga::IGenepoolPtr NNDriverApp::createGenepool()
{
	float PI = 3.14159265359f;

	std::vector<Body> worldBodies;
	worldBodies.push_back(Body({ 250.0f, 550.0f }, { 50.0f, 850.0f }, PI * 0.12f));
	worldBodies.push_back(Body({ 650.0f, 600.0f }, { 50.0f, 500.0f }, PI * 0.12f));
	worldBodies.push_back(Body({ 750.0f, 150.0f }, { 700.0f, 50.0f }));
	worldBodies.push_back(Body({ 800.0f, 550.0f }, { 400.0f, 50.0f }, PI * 0.4f));
	worldBodies.push_back(Body({ 1200.0f, 480.0f }, { 700.0f, 50.0f }, PI * 0.4f));

	std::vector<sf::Vector2f> targets;
	targets.push_back({ 580.0f, 265.0f });
	targets.push_back({ 970.0f, 265.0f });
	targets.push_back({ 1030.0f, 700.0f });
	targets.push_back({ 550.0f, 930.0f });
	targets.push_back({ 580.0f, 265.0f });
	targets.push_back({ 970.0f, 265.0f });
	targets.push_back({ 1030.0f, 700.0f });
	targets.push_back({ 550.0f, 930.0f });

	auto* pool = new NNDriverGenepool(
		[]()
	{
		return std::make_shared<NNGenome>(
			tbml::nn::NeuralNetwork({
				std::make_shared<tbml::nn::Layer::Dense>(8, 5),
				std::make_shared<tbml::nn::Layer::ReLU>(),
				std::make_shared<tbml::nn::Layer::Dense>(5, 2),
				std::make_shared<tbml::nn::Layer::TanH>()
				})
		);
	},
		nullptr,
		targets,
		40.0f,
		worldBodies
	);

	pool->setCreateAgentFn(
		[=](NNDriverGenepool::GenomeCPtr data)
	{
		return std::make_unique<NNDriverAgent>(
			std::move(data),
			pool,
			sf::Vector2f{ 380.0f, 780.0f },
			500.0f,
			20.0f,
			0.3f,
			0.98f,
			120.0f,
			300
		);
	});

	pool->configThreading(false, true, false);
	pool->resetGenepool(1000, 0.1f);

	return tbml::ga::IGenepoolPtr(pool);
}

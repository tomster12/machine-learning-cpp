#include "stdafx.h"
#include "TbmlGlobal.h"
#include "CommonImpl.h"
#include "NNDriverScenario.h"
#include "Body.h"
#include "UIStatsPanel.h"
#include "UITheme.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float MAX_DRIVING_SPEED = 1000.0f;
}

NNDriverScenario::NNDriverScenario()
	: nnRenderer(
		{ { "eye 1" }, { "eye 2" }, { "eye 3" }, { "eye 4" }, { "eye 5" }, { "speed", MAX_DRIVING_SPEED }, { "heading", PI, -1.0f }, { "target", PI } },
		{ "steer", "accel" })
{
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
			sf::Vector2f{ 300.0f, 780.0f }, MAX_DRIVING_SPEED, 25.0f, 0.5f, 0.95f, 120.0f, 0.25f, 300
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(2000, 0.05f);
	this->genepool->logInformation();
}

void NNDriverScenario::initAgentStats(UIStatsPanel& panel)
{
	panel.row("Step", [this]() { return std::to_string(stats.step); });

	// Swatches double as the legend for car colours
	panel.row("Alive", [this]() { return std::to_string(stats.alive) + " / " + std::to_string(stats.total); }, &theme::CAR_ALIVE);
	panel.row("Crashed", [this]() { return std::to_string(stats.crashed); }, &theme::CAR_CRASHED);
	panel.row("Timed out", [this]() { return std::to_string(stats.timedOut); }, &theme::CAR_TIMED_OUT);
	panel.row("Reached all", [this]() { return std::to_string(stats.completed); }, &theme::CAR_COMPLETED);
	panel.row("Furthest target", [this]() { return std::to_string(stats.furthestTarget) + " / " + std::to_string(genepool->getTargetCount()); });
}

void NNDriverScenario::updateDisplay()
{
	Scenario::updateDisplay();

	if (!genepool->getGenepoolInitialized()) return;

	this->updateStats();
	this->updateTrail();
}

void NNDriverScenario::updateStats()
{
	stats = PopulationStats();
	for (const auto& agent : *genepool->getAgentPopulation())
	{
		stats.total++;
		stats.furthestTarget = std::max(stats.furthestTarget, agent->getCurrentTarget());
		stats.step = std::max(stats.step, agent->getCurrentIteration());

		if (!agent->getFinished()) stats.alive++;
		else if (agent->getFinishType() == 0) stats.crashed++;
		else if (agent->getFinishType() == 1) stats.timedOut++;
		else stats.completed++;
	}
}

void NNDriverScenario::updateTrail()
{
	// Start a fresh trail whenever the elite changes
	const int generation = genepool->getGenerationNumber();
	if (getEliteIndex() != trailIndex || generation != trailGeneration)
	{
		trail.clear();
		trailIndex = getEliteIndex();
		trailGeneration = generation;
	}

	const sf::Vector2f pos = getEliteAgent()->getPosition();
	if (trail.empty() || (pos - trail.back()).lengthSquared() > 9.0f) trail.push_back(pos);
	while (trail.size() > 200) trail.pop_front();
}

void NNDriverScenario::renderWorld(sf::RenderWindow* window)
{
	// Grass with mowed stripes, covering whatever the camera can see
	const sf::View& view = window->getView();
	const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
	theme::drawRect(window, topLeft, view.getSize(), sf::Color(66, 128, 62));
	for (float x = std::floor(topLeft.x / 200.0f) * 200.0f; x < topLeft.x + view.getSize().x; x += 200.0f)
	{
		theme::drawRect(window, { x, topLeft.y }, { 100.0f, view.getSize().y }, sf::Color(0, 40, 0, 28));
	}

	genepool->renderWorld(window, getEliteAgent().get());
}

void NNDriverScenario::renderAgents(sf::RenderWindow* window)
{
	if (getView() != AgentView::NONE) this->renderTrail(window);

	genepool->renderAgents(window, getEliteAgent().get(), getView());
}

void NNDriverScenario::renderTrail(sf::RenderWindow* window)
{
	if (trail.size() < 2) return;

	sf::VertexArray line(sf::PrimitiveType::LineStrip);
	for (size_t i = 0; i < trail.size(); i++)
	{
		const float t = static_cast<float>(i) / static_cast<float>(trail.size() - 1);
		line.append(sf::Vertex{ trail[i], sf::Color(255, 255, 255, static_cast<std::uint8_t>(110.0f * t)) });
	}
	window->draw(line);
}

void NNDriverScenario::renderOverlay(sf::RenderWindow* window)
{
	const NNDriverGenepool::AgentPtr elite = getEliteAgent();
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

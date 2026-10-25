#include "stdafx.h"
#include <random>
#include "CommonImpl.h"
#include "NNTargetScenario.h"
#include "UIStatsPanel.h"
#include "UITheme.h"
#include "Utility.h"

namespace
{
	constexpr float TIME_STEP = 1.0f / 60.0f;
	constexpr int COURSE_PERIOD = 2;
	constexpr int RAMP_COURSES = 30;
	constexpr size_t TARGET_COUNT = 5;
	constexpr float AGENT_RADIUS = 6.0f;
	constexpr float MIN_LEG = 320.0f;
	const sf::Vector2f START_POS(700.0f, 850.0f);

	float randomRange(float low, float high) { return low + (high - low) * tbml::fn::getRandomFloat(); }

	int courseFor(int generation) { return (generation - 1) / COURSE_PERIOD + 1; }

	float difficultyFor(int number)
	{
		return std::min(1.0f, static_cast<float>(number - 1) / static_cast<float>(RAMP_COURSES));
	}

	float targetRadiusFor(int number) { return std::lerp(36.0f, 12.0f, difficultyFor(number)); }

	NNTargetAgent::Targets makeLayout(int number)
	{
		std::mt19937 rng(static_cast<unsigned int>(number) * 7919u + 17u);
		auto random = [&](float low, float high) { return low + (high - low) * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng); };

		// Scatter targets over the arena, each well clear of the one before (and the last clear of the first, as the route loops)
		NNTargetAgent::Targets targets;
		sf::Vector2f previous = START_POS;
		while (targets.size() < TARGET_COUNT)
		{
			const sf::Vector2f candidate(random(120.0f, 1280.0f), random(120.0f, 880.0f));
			if ((candidate - previous).length() < MIN_LEG) continue;
			if (targets.size() == TARGET_COUNT - 1 && (candidate - targets.front()).length() < MIN_LEG) continue;
			targets.push_back(candidate);
			previous = candidate;
		}
		return targets;
	}

}

NNTargetScenario::NNTargetScenario()
	: nnRenderer({ { "target x", 500.0f }, { "target y", 500.0f }, { "speed x", 500.0f }, { "speed y", 500.0f } }, { "x acc", "y acc" })
{
	this->genepool = std::make_shared<TargetGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(4, 2),
			std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr);

	// Every agent in a generation chases the same route, which is reshuffled every couple of generations as the targets shrink
	this->genepool->setCreateAgentFn([this](TargetGenepool::GenomeCPtr data)
	{
		const int number = courseFor(this->genepool->getGenerationNumber() + 1);
		return std::make_unique<NNTargetAgent>(
			std::move(data), this->getTargets(number), targetRadiusFor(number),
			START_POS, AGENT_RADIUS, 400.0f, 0.99f, 3000
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(1000, 0.1f);
}

float NNTargetScenario::getDifficulty(int generation) const
{
	return difficultyFor(courseFor(generation));
}

std::shared_ptr<const NNTargetAgent::Targets> NNTargetScenario::getTargets(int number)
{
	if (targetsCache == nullptr || targetsNumber != number)
	{
		targetsCache = std::make_shared<const NNTargetAgent::Targets>(makeLayout(number));
		targetsNumber = number;
	}
	return targetsCache;
}

void NNTargetScenario::initAgentStats(UIStatsPanel& panel)
{
	panel.row("Step", [this]() { return std::to_string(stats.step); });
	panel.row("Speed", [this]() { return global::formatFloat(getEliteAgent()->getVelocity().length(), 0); });
	panel.row("Targets hit", [this]() { return std::to_string(getEliteAgent()->getTargetsHit()); });
	panel.row("Furthest target", [this]() { return std::to_string(stats.furthestTarget); });

	panel.header("Course");
	panel.row("Number", [this]() { return std::to_string(courseFor(genepool->getGenerationNumber())); });
	panel.row("Difficulty", [this]() { return global::formatFloat(this->getDifficulty(genepool->getGenerationNumber()) * 100.0f, 0) + "%"; });
	panel.row("Target size", [this]() { return global::formatFloat(getEliteAgent()->getTargetRadius(), 0); });

	// Swatch doubles as the legend for agent colours
	panel.header("Population");
	panel.row("Running", [this]() { return std::to_string(stats.running) + " / " + std::to_string(stats.total); }, &theme::CAR_ALIVE);
}

void NNTargetScenario::updateDisplay()
{
	Scenario::updateDisplay();

	if (!genepool->getGenepoolInitialized()) return;

	this->updateStats();
	this->updateTrail();
	this->updateEffects();
}

void NNTargetScenario::updateStats()
{
	stats = PopulationStats();
	for (const auto& agent : *genepool->getAgentPopulation())
	{
		stats.total++;
		stats.furthestTarget = std::max(stats.furthestTarget, agent->getTargetsHit());
		stats.step = std::max(stats.step, agent->getCurrentIteration());
		if (!agent->getFinished()) stats.running++;
	}
}

void NNTargetScenario::updateTrail()
{
	const NNTargetAgent& elite = *getEliteAgent();
	if (trail.follow(getEliteIndex(), genepool->getGenerationNumber(), elite.getPosition())) lastHits = elite.getTargetsHit();
}

void NNTargetScenario::updateEffects()
{
	effects.update(TIME_STEP);

	const NNTargetAgent& elite = *getEliteAgent();

	// Thruster sparks fly out opposite to where the elite is accelerating
	const sf::Vector2f acc = elite.getAcceleration();
	const float thrust = acc.length();
	if (!elite.getFinished() && thrust > 0.15f)
	{
		const sf::Vector2f direction = acc / thrust;
		const sf::Vector2f sideways(-direction.y, direction.x);
		for (int i = 0; i < 2; i++)
		{
			const sf::Vector2f pos = elite.getPosition() - direction * elite.getRadius();
			const sf::Vector2f vel = -direction * randomRange(60.0f, 150.0f) * (0.5f + thrust) + sideways * randomRange(-25.0f, 25.0f) + elite.getVelocity() * 0.2f;
			const sf::Color colour = sf::Color(static_cast<std::uint8_t>(randomRange(130.0f, 190.0f)), 210, 255);
			effects.emit(pos, vel, colour, randomRange(0.25f, 0.5f), randomRange(1.5f, 3.5f));
		}
	}

	// Ripple out of a target whenever the elite reaches it
	const size_t hits = elite.getTargetsHit();
	if (hits > lastHits) effects.ripple(elite.getTarget(hits - 1), elite.getTargetRadius(), theme::INFO);
	lastHits = hits;
}

void NNTargetScenario::renderWorld(sf::RenderWindow* window)
{
	this->renderBackdrop(window);
	this->renderTargets(window, *getEliteAgent());
}

void NNTargetScenario::renderBackdrop(sf::RenderWindow* window) const
{
	// Vertical gradient across whatever the camera can see, with a faint grid on top
	const sf::View& view = window->getView();
	const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
	const sf::Vector2f size = view.getSize();
	const sf::Color top(12, 14, 22);
	const sf::Color bottom(26, 30, 44);

	sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
	sky.append({ topLeft, top });
	sky.append({ { topLeft.x + size.x, topLeft.y }, top });
	sky.append({ { topLeft.x, topLeft.y + size.y }, bottom });
	sky.append({ topLeft + size, bottom });
	window->draw(sky);

	const float spacing = 100.0f;
	const sf::Color gridColour(255, 255, 255, 10);
	sf::VertexArray grid(sf::PrimitiveType::Lines);
	for (float x = std::floor(topLeft.x / spacing) * spacing; x < topLeft.x + size.x; x += spacing)
	{
		grid.append({ { x, topLeft.y }, gridColour });
		grid.append({ { x, topLeft.y + size.y }, gridColour });
	}
	for (float y = std::floor(topLeft.y / spacing) * spacing; y < topLeft.y + size.y; y += spacing)
	{
		grid.append({ { topLeft.x, y }, gridColour });
		grid.append({ { topLeft.x + size.x, y }, gridColour });
	}
	window->draw(grid);

	theme::drawRing(window, START_POS, 14.0f, 1.5f, sf::Color(255, 255, 255, 40));
	theme::drawText(window, "start", { START_POS.x, START_POS.y + 30.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
}

void NNTargetScenario::renderTargets(sf::RenderWindow* window, const NNTargetAgent& elite) const
{
	const size_t count = elite.getTargetCount();
	const size_t current = elite.getTargetsHit() % count;
	const size_t next = (current + 1) % count;
	const float radius = elite.getTargetRadius();
	const float time = clock.getElapsedTime().asSeconds();

	// Route the targets are visited in, looping back round to the first
	sf::VertexArray route(sf::PrimitiveType::LineStrip);
	for (size_t i = 0; i <= count; i++) route.append({ elite.getTarget(i), sf::Color(255, 255, 255, 22) });
	window->draw(route);

	// Targets: bright and pulsing for the one being chased, dim for those already visited this lap
	for (size_t i = 0; i < count; i++)
	{
		const sf::Vector2f pos = elite.getTarget(i);
		const bool isCurrent = i == current;
		const bool visited = i < current;
		const sf::Color base = isCurrent ? theme::INFO : sf::Color(255, 255, 255);

		if (isCurrent)
		{
			const float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
			theme::drawRing(window, pos, radius * (1.5f + 0.2f * pulse), 2.0f, sf::Color(base.r, base.g, base.b, theme::toAlpha(40.0f + 50.0f * (1.0f - pulse))));
		}

		theme::drawRing(window, pos, radius, 2.0f, sf::Color(base.r, base.g, base.b, isCurrent ? 230 : visited ? 50 : i == next ? 150 : 110), sf::Color(base.r, base.g, base.b, isCurrent ? 50 : 16));
		theme::drawText(window, std::to_string(i + 1), pos, theme::TEXT_BODY, sf::Color(base.r, base.g, base.b, isCurrent ? 255 : visited ? 90 : 190), theme::Align::CENTER);
	}
}

void NNTargetScenario::renderAgents(sf::RenderWindow* window)
{
	if (getView() == AgentView::NONE) return;

	const NNTargetAgent* elite = getEliteAgent().get();

	trail.render(window);

	// Whole pack in one draw call, elite is drawn separately on top
	if (getView() == AgentView::ALL)
	{
		packVertices.clear();
		for (const auto& agent : *genepool->getAgentPopulation())
		{
			if (agent.get() != elite) agent->appendPackVertices(packVertices);
		}
		window->draw(packVertices);
	}

	effects.render(window);

	if (!elite->getFinished())
	{
		const sf::Color lineColour(255, 255, 255, 70);
		const sf::Vertex line[2] = { { elite->getPosition(), lineColour }, { elite->getTarget(elite->getTargetsHit()), lineColour } };
		window->draw(line, 2, sf::PrimitiveType::Lines);
	}
	elite->renderElite(window);
}

void NNTargetScenario::renderOverlay(sf::RenderWindow* window)
{
	const TargetGenepool::AgentPtr elite = getEliteAgent();
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

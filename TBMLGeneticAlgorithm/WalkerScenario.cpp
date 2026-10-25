#include "stdafx.h"
#include "TbmlGlobal.h"
#include "CommonImpl.h"
#include "WalkerScenario.h"
#include "UIStatsPanel.h"
#include "UITheme.h"
#include "Utility.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float TIME_STEP = 1.0f / 60.0f;
	constexpr int COURSE_PERIOD = 3;
	constexpr float RAD_TO_DEG = 57.2957795f;

	float randomRange(float low, float high) { return low + (high - low) * tbml::fn::getRandomFloat(); }

	sf::Color lerpColour(sf::Color a, sf::Color b, float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return sf::Color(
			static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
			static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
			static_cast<std::uint8_t>(a.b + (b.b - a.b) * t));
	}
}

WalkerScenario::WalkerScenario()
	: nnRenderer(
		{ { "tilt" }, { "spin" }, { "height" }, { "vx" }, { "vy" }, { "hip L" }, { "knee L" }, { "hip R" }, { "knee R" }, { "foot L" }, { "foot R" }, { "sin" }, { "cos" }, { "g+30" }, { "g+70" }, { "g+120" } },
		{ "hip L", "knee L", "hip R", "knee R" })
{
	this->genepool = std::make_shared<WalkerGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(WalkerAgent::INPUT_COUNT, 14),
			std::make_shared<tbml::nn::Layer::TanH>(),
			std::make_shared<tbml::nn::Layer::Dense>(14, 10),
			std::make_shared<tbml::nn::Layer::TanH>(),
			std::make_shared<tbml::nn::Layer::Dense>(10, WalkerAgent::JOINT_COUNT),
			std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr);

	// Every agent in a generation walks the same ground, which gets bumpier every few generations
	this->genepool->setCreateAgentFn([this](WalkerGenepool::GenomeCPtr data)
	{
		return std::make_unique<WalkerAgent>(std::move(data), this->getCourse(this->genepool->getGenerationNumber() / COURSE_PERIOD + 1));
	});

	// Squared fitness already sharpens roulette
	this->genepool->configSelection(tbml::ga::SelectionType::ROULETTE);
	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(600, 0.06f);
	this->genepool->logInformation();

	this->buildHills();
}

float WalkerScenario::getDifficulty(int generation) const
{
	return WalkerCourse::getDifficultyFor((generation - 1) / COURSE_PERIOD + 1);
}

std::shared_ptr<const WalkerCourse> WalkerScenario::getCourse(int number)
{
	if (courseCache == nullptr || courseCache->getNumber() != number) courseCache = std::make_shared<const WalkerCourse>(number);
	return courseCache;
}

void WalkerScenario::buildHills()
{
	// Two layers of silhouette hills behind the course, built once
	for (int layer = 0; layer < 2; layer++)
	{
		sf::VertexArray& hills = layer == 0 ? farHills : nearHills;
		const float base = layer == 0 ? 600.0f : 690.0f;
		const float amplitude = layer == 0 ? 70.0f : 45.0f;
		const float frequency = layer == 0 ? 0.0045f : 0.007f;
		const float phase = randomRange(0.0f, 6.28f);
		const sf::Color top = layer == 0 ? sf::Color(52, 44, 84) : sf::Color(40, 38, 66);
		const sf::Color bottom = layer == 0 ? sf::Color(30, 28, 56) : sf::Color(24, 24, 44);
		for (float x = -400.0f; x <= 1800.0f; x += 20.0f)
		{
			const float y = base + amplitude * (std::sin(x * frequency + phase) + 0.4f * std::sin(x * frequency * 2.7f + phase * 1.9f));
			hills.append({ { x, y }, top });
			hills.append({ { x, 1800.0f }, bottom });
		}
	}
}

void WalkerScenario::initAgentStats(UIStatsPanel& panel)
{
	panel.row("Time", [this]() { return global::formatFloat(static_cast<float>(getEliteAgent()->getStep()) * WalkerAgent::TIME_STEP, 1) + " / " + global::formatFloat(static_cast<float>(WalkerAgent::MAX_STEPS) * WalkerAgent::TIME_STEP, 0) + " s"; });
	panel.row("Distance", [this]() { return global::formatFloat(getEliteAgent()->getDistance(), 1) + " / " + global::formatFloat((WalkerCourse::GOAL_X - WalkerCourse::START_X) / WalkerAgent::PIXELS_PER_METRE, 0) + " m"; });
	panel.row("Speed", [this]() { return global::formatFloat(getEliteAgent()->getHipVelocity().x / WalkerAgent::PIXELS_PER_METRE, 1) + " m/s"; });
	panel.row("Torso lean", [this]() { return global::formatFloat(getEliteAgent()->getTilt() * RAD_TO_DEG, 0) + " deg"; });

	panel.header("Course");
	panel.row("Number", [this]() { return std::to_string(getEliteAgent()->getCourse()->getNumber()); });
	panel.row("Difficulty", [this]() { return global::formatFloat(getEliteAgent()->getCourse()->getDifficulty() * 100.0f, 0) + "%"; });
	panel.row("Bumpiness", [this]() { return global::formatFloat(getEliteAgent()->getCourse()->getRoughness(), 0); });

	// Swatches double as the legend for walker colours
	panel.header("Outcomes");
	panel.row("Walking", [this]() { return std::to_string(stats.walking) + " / " + std::to_string(stats.total); }, &theme::WALKER_WALKING);
	panel.row("Fell", [this]() { return std::to_string(stats.fell); }, &theme::WALKER_FELL);
	panel.row("Timed out", [this]() { return std::to_string(stats.timedOut); }, &theme::WALKER_TIMED_OUT);
	panel.row("Finished", [this]() { return std::to_string(stats.finished); }, &theme::WALKER_FINISHED);
	panel.row("Furthest", [this]() { return global::formatFloat(stats.furthest, 1) + " m"; });
}

void WalkerScenario::updateDisplay()
{
	Scenario::updateDisplay();

	if (!genepool->getGenepoolInitialized()) return;

	this->updateStats();
	trail.follow(getEliteIndex(), genepool->getGenerationNumber(), getEliteAgent()->getPoint(WalkerAgent::HIP));
	this->updateParticles();
}

void WalkerScenario::updateStats()
{
	stats = PopulationStats();
	for (const auto& agent : *genepool->getAgentPopulation())
	{
		stats.total++;
		stats.step = std::max(stats.step, agent->getStep());
		stats.furthest = std::max(stats.furthest, agent->getDistance());

		switch (agent->getOutcome())
		{
		case WalkerOutcome::WALKING: stats.walking++; break;
		case WalkerOutcome::FELL: stats.fell++; break;
		case WalkerOutcome::TIMED_OUT: stats.timedOut++; break;
		case WalkerOutcome::FINISHED: stats.finished++; break;
		}
	}
}

void WalkerScenario::updateParticles()
{
	effects.update(TIME_STEP);

	const WalkerAgent& elite = *getEliteAgent();

	// Puff of dust every time a foot lands
	const size_t feet[2] = { WalkerAgent::FOOT_L, WalkerAgent::FOOT_R };
	for (size_t leg = 0; leg < 2; leg++)
	{
		const bool contact = elite.getFootContact(leg);
		if (contact && !lastContact[leg] && !elite.getFinished())
		{
			for (int i = 0; i < 5; i++)
			{
				const float angle = -PI * randomRange(0.1f, 0.9f);
				effects.emit(elite.getPoint(feet[leg]), sf::Vector2f(std::cos(angle), std::sin(angle)) * randomRange(15.0f, 60.0f), sf::Color(190, 182, 160), randomRange(0.3f, 0.7f), randomRange(1.5f, 3.0f), 120.0f);
			}
		}
		lastContact[leg] = contact;
	}

	// One burst when the elite's run ends
	if (!elite.getFinished()) burstDone = false;
	else if (!burstDone)
	{
		burstDone = true;
		if (elite.getOutcome() == WalkerOutcome::TIMED_OUT) return;

		const bool won = elite.getOutcome() == WalkerOutcome::FINISHED;
		const sf::Vector2f origin = elite.getPoint(WalkerAgent::HIP);
		for (int i = 0; i < 60; i++)
		{
			// Wins throw confetti upwards, falls throw sparks in every direction
			const float angle = won ? -PI * randomRange(0.15f, 0.85f) : randomRange(0.0f, 2.0f * PI);
			const float speed = won ? randomRange(80.0f, 300.0f) : randomRange(30.0f, 200.0f);
			const sf::Color colour = won ? (i % 2 == 0 ? theme::INFO : theme::TEXT_PRIMARY) : (i % 3 == 0 ? theme::NEGATIVE : sf::Color(255, 170, 80));
			effects.emit(origin, sf::Vector2f(std::cos(angle), std::sin(angle)) * speed, colour, randomRange(0.6f, 1.6f), randomRange(1.5f, 3.5f), 260.0f);
		}
	}
}

void WalkerScenario::renderWorld(sf::RenderWindow* window)
{
	const WalkerCourse& course = *getEliteAgent()->getCourse();

	this->renderSky(window);
	this->renderTerrain(window, course);
	this->renderMarkers(window, course);
}

void WalkerScenario::renderSky(sf::RenderWindow* window) const
{
	// Dusk gradient across whatever the camera can see, a moon, then the hills
	const sf::View& view = window->getView();
	const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
	const sf::Vector2f size = view.getSize();
	const sf::Color top(14, 16, 40);
	const sf::Color bottom(120, 74, 104);
	const float t0 = topLeft.y / WalkerCourse::HEIGHT;
	const float t1 = (topLeft.y + size.y) / WalkerCourse::HEIGHT;

	sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
	sky.append({ topLeft, lerpColour(top, bottom, t0 * 1.1f) });
	sky.append({ { topLeft.x + size.x, topLeft.y }, lerpColour(top, bottom, t0 * 1.1f) });
	sky.append({ { topLeft.x, topLeft.y + size.y }, lerpColour(top, bottom, t1 * 1.1f) });
	sky.append({ topLeft + size, lerpColour(top, bottom, t1 * 1.1f) });
	window->draw(sky);

	const float pulse = 0.5f + 0.5f * std::sin(clock.getElapsedTime().asSeconds() * 0.5f);
	const sf::Vector2f moon(1100.0f, 230.0f);
	sf::VertexArray glow(sf::PrimitiveType::Triangles);
	theme::appendDisc(glow, moon, 120.0f + 10.0f * pulse, sf::Color(255, 226, 200, 14), 32);
	theme::appendDisc(glow, moon, 78.0f, sf::Color(255, 232, 210, 26), 32);
	theme::appendDisc(glow, moon, 42.0f, sf::Color(250, 244, 226, 235), 32);
	window->draw(glow);

	window->draw(farHills);
	window->draw(nearHills);
}

void WalkerScenario::renderTerrain(sf::RenderWindow* window, const WalkerCourse& course)
{
	// Rebuild the ground only when the course changes
	if (terrainNumber != course.getNumber())
	{
		terrainFill.clear();
		terrainEdge.clear();
		const std::vector<float>& heights = course.getHeights();
		for (size_t i = 0; i < heights.size(); i++)
		{
			const float x = static_cast<float>(i) * WalkerCourse::POINT_SPACING;
			terrainFill.append({ { x, heights[i] }, sf::Color(70, 82, 70) });
			terrainFill.append({ { x, 1800.0f }, sf::Color(16, 20, 22) });
			terrainEdge.append({ { x, heights[i] }, sf::Color(168, 206, 148, 240) });
		}
		terrainNumber = course.getNumber();
	}

	window->draw(terrainFill);
	window->draw(terrainEdge);
}

void WalkerScenario::renderMarkers(sf::RenderWindow* window, const WalkerCourse& course) const
{
	const float time = clock.getElapsedTime().asSeconds();

	// Distance ticks along the ground every five metres
	const float spacing = 5.0f * WalkerAgent::PIXELS_PER_METRE;
	int metres = 0;
	for (float x = WalkerCourse::START_X; x <= WalkerCourse::GOAL_X + 1.0f; x += spacing, metres += 5)
	{
		const float y = course.getHeight(x);
		theme::drawRect(window, { x - 1.0f, y }, { 2.0f, 10.0f }, sf::Color(255, 255, 255, 70));
		theme::drawText(window, std::to_string(metres) + " m", { x, y + 24.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
	}

	// Start line
	const float startY = course.getHeight(WalkerCourse::START_X);
	theme::drawRect(window, { WalkerCourse::START_X - 1.0f, startY - 60.0f }, { 2.0f, 60.0f }, sf::Color(255, 255, 255, 50));

	// Finish gate: a pole with a checkered banner and a beam of light
	const float goalY = course.getHeight(WalkerCourse::GOAL_X);
	const float poleTop = goalY - 150.0f;
	sf::VertexArray beam(sf::PrimitiveType::TriangleStrip);
	beam.append({ { WalkerCourse::GOAL_X - 40.0f, goalY }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 46) });
	beam.append({ { WalkerCourse::GOAL_X + 40.0f, goalY }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 46) });
	beam.append({ { WalkerCourse::GOAL_X - 28.0f, goalY - 300.0f }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 0) });
	beam.append({ { WalkerCourse::GOAL_X + 28.0f, goalY - 300.0f }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 0) });
	window->draw(beam);

	theme::drawRect(window, { WalkerCourse::GOAL_X - 2.0f, poleTop }, { 4.0f, 150.0f }, theme::INFO);
	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col < 6; col++)
		{
			// Banner ripples in the wind
			const float wave = 3.0f * std::sin(time * 4.0f + static_cast<float>(col) * 0.9f);
			const sf::Color colour = (row + col) % 2 == 0 ? theme::TEXT_PRIMARY : sf::Color(30, 32, 40);
			theme::drawRect(window, { WalkerCourse::GOAL_X + 2.0f + static_cast<float>(col) * 9.0f, poleTop + static_cast<float>(row) * 9.0f + wave }, { 9.0f, 9.0f }, colour);
		}
	}
}

void WalkerScenario::renderAgents(sf::RenderWindow* window)
{
	if (getView() == AgentView::NONE) return;

	const WalkerAgent* elite = getEliteAgent().get();

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
	elite->renderElite(window);
}

void WalkerScenario::renderOverlay(sf::RenderWindow* window)
{
	const WalkerGenepool::AgentPtr elite = getEliteAgent();
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

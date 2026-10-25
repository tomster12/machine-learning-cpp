#include "stdafx.h"
#include "TbmlGlobal.h"
#include "CommonImpl.h"
#include "LanderScenario.h"
#include "UIStatsPanel.h"
#include "UITheme.h"
#include "Utility.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float TIME_STEP = 1.0f / 60.0f;
	constexpr int COURSE_PERIOD = 3;

	float randomRange(float low, float high) { return low + (high - low) * tbml::fn::getRandomFloat(); }

	sf::Color skyColour(float t)
	{
		const sf::Color top(5, 7, 20);
		const sf::Color bottom(34, 26, 58);
		t = std::clamp(t, 0.0f, 1.0f);
		return sf::Color(
			static_cast<std::uint8_t>(top.r + (bottom.r - top.r) * t),
			static_cast<std::uint8_t>(top.g + (bottom.g - top.g) * t),
			static_cast<std::uint8_t>(top.b + (bottom.b - top.b) * t));
	}
}

LanderScenario::LanderScenario()
	: nnRenderer(
		{ { "dx" }, { "dy" }, { "vx" }, { "vy" }, { "tilt" }, { "spin" }, { "fuel" }, { "r-90" }, { "r-60" }, { "r-30" }, { "r0" }, { "r+30" }, { "r+60" }, { "r+90" } },
		{ "thrust", "turn" })
{
	this->genepool = std::make_shared<LanderGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(LanderAgent::INPUT_COUNT, 16),
			std::make_shared<tbml::nn::Layer::TanH>(),
			std::make_shared<tbml::nn::Layer::Dense>(16, 12),
			std::make_shared<tbml::nn::Layer::TanH>(),
			std::make_shared<tbml::nn::Layer::Dense>(12, 2),
			std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr);

	// Every agent in a generation flies the same course, which changes every few generations as it gets harder
	this->genepool->setCreateAgentFn([this](LanderGenepool::GenomeCPtr data)
	{
		return std::make_unique<LanderAgent>(std::move(data), this->getCourse(this->genepool->getGenerationNumber() / COURSE_PERIOD + 1));
	});

	// Squared fitness already sharpens roulette, which beat tournaments here
	this->genepool->configSelection(tbml::ga::SelectionType::ROULETTE);
	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(1500, 0.05f);
	this->genepool->logInformation();

	for (int i = 0; i < 160; i++)
	{
		stars.push_back({ { randomRange(-300.0f, 1700.0f), randomRange(-300.0f, 800.0f) }, randomRange(0.6f, 1.8f), randomRange(0.0f, 6.28f) });
	}
}

float LanderScenario::getDifficulty(int generation) const
{
	return LanderCourse::getDifficultyFor((generation - 1) / COURSE_PERIOD + 1);
}

std::shared_ptr<const LanderCourse> LanderScenario::getCourse(int number)
{
	if (courseCache == nullptr || courseCache->getNumber() != number) courseCache = std::make_shared<const LanderCourse>(number);
	return courseCache;
}

void LanderScenario::initAgentStats(UIStatsPanel& panel)
{
	panel.row("Step", [this]() { return std::to_string(stats.step); });
	panel.row("Fuel", [this]() { return global::formatFloat(getEliteAgent()->getFuel() * 100.0f, 0) + "%"; });
	panel.row("Speed", [this]() { return global::formatFloat(getEliteAgent()->getVelocity().length(), 0); });

	panel.header("Course");
	panel.row("Number", [this]() { return std::to_string(getEliteAgent()->getCourse()->getNumber()); });
	panel.row("Difficulty", [this]() { return global::formatFloat(getEliteAgent()->getCourse()->getDifficulty() * 100.0f, 0) + "%"; });
	panel.row("Pad width", [this]() { return global::formatFloat(getEliteAgent()->getCourse()->getPadWidth(), 0); });

	// Swatches double as the legend for ship colours
	panel.header("Outcomes");
	panel.row("Flying", [this]() { return std::to_string(stats.flying) + " / " + std::to_string(stats.total); }, &theme::SHIP_FLYING);
	panel.row("Landed", [this]() { return std::to_string(stats.landed); }, &theme::SHIP_LANDED);
	panel.row("Crashed", [this]() { return std::to_string(stats.crashed); }, &theme::SHIP_CRASHED);
	panel.row("Lost", [this]() { return std::to_string(stats.lost); }, &theme::SHIP_LOST);
	panel.row("Landing rate", [this]() { return global::formatFloat(100.0f * static_cast<float>(stats.landed) / static_cast<float>(std::max<size_t>(1, stats.total)), 1) + "%"; });
}

void LanderScenario::updateDisplay()
{
	Scenario::updateDisplay();

	if (!genepool->getGenepoolInitialized()) return;

	this->updateStats();
	this->updateTrail();
	this->updateParticles();
}

void LanderScenario::updateStats()
{
	stats = PopulationStats();
	for (const auto& agent : *genepool->getAgentPopulation())
	{
		stats.total++;
		stats.step = std::max(stats.step, agent->getStep());

		switch (agent->getOutcome())
		{
		case LanderOutcome::FLYING: stats.flying++; break;
		case LanderOutcome::LANDED: stats.landed++; break;
		case LanderOutcome::CRASHED: stats.crashed++; break;
		case LanderOutcome::LOST: stats.lost++; break;
		}
	}
}

void LanderScenario::updateTrail()
{
	trail.follow(getEliteIndex(), genepool->getGenerationNumber(), getEliteAgent()->getPosition());
}

void LanderScenario::updateParticles()
{
	effects.update(TIME_STEP);

	const LanderAgent& elite = *getEliteAgent();

	// Exhaust trails off the elite's engine
	if (!elite.getFinished() && elite.getThrottle() > 0.05f)
	{
		for (int i = 0; i < 3; i++)
		{
			const sf::Vector2f direction = elite.getExhaustDirection();
			const sf::Vector2f sideways(-direction.y, direction.x);
			const sf::Vector2f vel = direction * randomRange(160.0f, 260.0f) * (0.5f + elite.getThrottle()) + sideways * randomRange(-35.0f, 35.0f) + elite.getVelocity() * 0.3f;
			const sf::Color colour(255, static_cast<std::uint8_t>(randomRange(120.0f, 210.0f)), 70);
			effects.emit(elite.getNozzlePosition(), vel, colour, randomRange(0.3f, 0.6f), randomRange(1.5f, 3.5f));
		}
	}

	// One burst when the elite touches down
	if (!elite.getFinished()) burstDone = false;
	else if (!burstDone)
	{
		this->spawnBurst(elite);
		burstDone = true;
	}
}

void LanderScenario::spawnBurst(const LanderAgent& elite)
{
	const bool landed = elite.getOutcome() == LanderOutcome::LANDED;
	if (elite.getOutcome() == LanderOutcome::LOST) return;

	const int count = landed ? 40 : 80;
	for (int i = 0; i < count; i++)
	{
		// Landings kick up low dust, crashes throw sparks in every direction
		const float angle = landed ? -PI * randomRange(0.05f, 0.95f) : randomRange(0.0f, 2.0f * PI);
		const float speed = landed ? randomRange(20.0f, 90.0f) : randomRange(40.0f, 260.0f);
		const sf::Color colour = landed ? sf::Color(200, 225, 250) : (i % 3 == 0 ? theme::NEGATIVE : sf::Color(255, 170, 80));
		const float life = landed ? randomRange(0.6f, 1.2f) : randomRange(0.6f, 1.6f);
		effects.emit(elite.getPosition(), sf::Vector2f(std::cos(angle), std::sin(angle)) * speed, colour, life, randomRange(1.5f, 3.5f), landed ? 20.0f : 200.0f);
	}
}

void LanderScenario::renderWorld(sf::RenderWindow* window)
{
	const LanderCourse& course = *getEliteAgent()->getCourse();

	this->renderSky(window);
	this->renderTerrain(window, course);
	this->renderPad(window, course);
}

void LanderScenario::renderSky(sf::RenderWindow* window) const
{
	// Vertical gradient across whatever the camera can see, then twinkling stars
	const sf::View& view = window->getView();
	const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
	const sf::Vector2f size = view.getSize();
	const sf::Color top = skyColour(topLeft.y / LanderCourse::HEIGHT);
	const sf::Color bottom = skyColour((topLeft.y + size.y) / LanderCourse::HEIGHT);

	sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
	sky.append({ topLeft, top });
	sky.append({ { topLeft.x + size.x, topLeft.y }, top });
	sky.append({ { topLeft.x, topLeft.y + size.y }, bottom });
	sky.append({ topLeft + size, bottom });
	window->draw(sky);

	const float time = clock.getElapsedTime().asSeconds();
	sf::VertexArray starVertices(sf::PrimitiveType::Triangles);
	for (const Star& star : stars)
	{
		const float twinkle = 0.55f + 0.45f * std::sin(time * 1.5f + star.phase);
		theme::appendDisc(starVertices, star.pos, star.size * 0.5f, sf::Color(220, 230, 255, theme::toAlpha(200.0f * twinkle)), 6);
	}
	window->draw(starVertices);
}

void LanderScenario::renderTerrain(sf::RenderWindow* window, const LanderCourse& course)
{
	// Rebuild the ground only when the course changes
	if (terrainNumber != course.getNumber())
	{
		terrainFill.clear();
		terrainEdge.clear();
		const std::vector<float>& heights = course.getHeights();
		for (size_t i = 0; i < heights.size(); i++)
		{
			const float x = static_cast<float>(i) * LanderCourse::POINT_SPACING;
			terrainFill.append({ { x, heights[i] }, sf::Color(92, 84, 106) });
			terrainFill.append({ { x, 1800.0f }, sf::Color(20, 18, 30) });
			terrainEdge.append({ { x, heights[i] }, sf::Color(200, 192, 218, 230) });
		}
		terrainNumber = course.getNumber();
	}

	window->draw(terrainFill);
	window->draw(terrainEdge);
}

void LanderScenario::renderPad(sf::RenderWindow* window, const LanderCourse& course) const
{
	const sf::Vector2f centre = course.getPadCentre();
	const float half = course.getPadWidth() / 2.0f;
	const float time = clock.getElapsedTime().asSeconds();

	// Beam of light rising from the pad
	sf::VertexArray beam(sf::PrimitiveType::TriangleStrip);
	beam.append({ { centre.x - half, centre.y }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 50) });
	beam.append({ { centre.x + half, centre.y }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 50) });
	beam.append({ { centre.x - half * 0.7f, centre.y - 320.0f }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 0) });
	beam.append({ { centre.x + half * 0.7f, centre.y - 320.0f }, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 0) });
	window->draw(beam);

	theme::drawRect(window, { centre.x - half, centre.y - 3.0f }, { half * 2.0f, 6.0f }, theme::INFO);

	// Beacons at each end, blinking out of phase
	sf::CircleShape beacon(5.0f);
	beacon.setOrigin({ 5.0f, 5.0f });
	for (int side = -1; side <= 1; side += 2)
	{
		const float blink = 0.5f + 0.5f * std::sin(time * 4.0f + static_cast<float>(side) * 1.6f);
		beacon.setPosition({ centre.x + static_cast<float>(side) * half, centre.y - 8.0f });
		beacon.setFillColor(sf::Color(255, 120, 110, theme::toAlpha(90.0f + 165.0f * blink)));
		window->draw(beacon);
	}
}

void LanderScenario::renderAgents(sf::RenderWindow* window)
{
	if (getView() == AgentView::NONE) return;

	const LanderAgent* elite = getEliteAgent().get();

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
		const sf::Color lineColour(255, 255, 255, 60);
		const sf::Vertex line[2] = { { elite->getPosition(), lineColour }, { elite->getCourse()->getPadCentre(), lineColour } };
		window->draw(line, 2, sf::PrimitiveType::Lines);
	}
	elite->renderElite(window);
}

void LanderScenario::renderOverlay(sf::RenderWindow* window)
{
	const LanderGenepool::AgentPtr elite = getEliteAgent();
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

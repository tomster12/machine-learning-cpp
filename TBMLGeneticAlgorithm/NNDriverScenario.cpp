#include "stdafx.h"
#include "TbmlGlobal.h"
#include "CommonImpl.h"
#include "NNDriverScenario.h"
#include "UIStatsPanel.h"
#include "UITheme.h"
#include "Utility.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float MAX_DRIVING_SPEED = 500.0f;
	constexpr float TIME_STEP = 1.0f / 60.0f;
	constexpr int COURSE_PERIOD = 3;

	float randomRange(float low, float high) { return low + (high - low) * tbml::fn::getRandomFloat(); }
}

NNDriverScenario::NNDriverScenario()
	: nnRenderer(
		{ { "eye 1" }, { "eye 2" }, { "eye 3" }, { "eye 4" }, { "eye 5" }, { "speed" }, { "target dist" }, { "target angle" } },
		{ "steer", "accel" })
{
	this->genepool = std::make_shared<DriverGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
				std::make_shared<tbml::nn::Layer::Dense>(NNDriverAgent::INPUT_COUNT, 6),
				std::make_shared<tbml::nn::Layer::ReLU>(),
				std::make_shared<tbml::nn::Layer::Dense>(6, 5),
				std::make_shared<tbml::nn::Layer::ReLU>(),
				std::make_shared<tbml::nn::Layer::Dense>(5, 2),
				std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr);

	// Every agent in a generation drives the same road, which is replaced every few generations as it gets harder
	this->genepool->setCreateAgentFn(
		[this](DriverGenepool::GenomeCPtr data)
	{
		return std::make_unique<NNDriverAgent>(
			std::move(data), this->getCourse(this->genepool->getGenerationNumber() / COURSE_PERIOD + 1),
			MAX_DRIVING_SPEED, 25.0f, 0.1f, 0.95f, 200.0f, 0.4f, 300
		);
	});

	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(2000, 0.05f);
	this->genepool->logInformation();
}

float NNDriverScenario::getGenerationDifficulty(int generation) const
{
	return DriverCourse::getGenerationDifficulty((generation - 1) / COURSE_PERIOD + 1);
}

std::shared_ptr<const DriverCourse> NNDriverScenario::getCourse(int number)
{
	if (courseCache == nullptr || courseCache->getNumber() != number) courseCache = std::make_shared<const DriverCourse>(number);
	return courseCache;
}

void NNDriverScenario::initAgentStats(UIStatsPanel& panel)
{
	panel.row("Step", [this]() { return std::to_string(stats.step); });
	panel.row("Speed", [this]() { return global::formatFloat(getEliteAgent()->getSpeed(), 0); });
	panel.row("Elite target", [this]() { return std::to_string(getEliteAgent()->getCurrentTarget()) + " / " + std::to_string(getEliteAgent()->getCourse()->getTargetCount()); });

	panel.header("Course");
	panel.row("Number", [this]() { return std::to_string(getEliteAgent()->getCourse()->getNumber()); });
	panel.row("Difficulty", [this]() { return global::formatFloat(getEliteAgent()->getCourse()->getGenerationDifficulty() * 100.0f, 0) + "%"; });
	panel.row("Road width", [this]() { const DriverCourse& course = *getEliteAgent()->getCourse(); return global::formatFloat(course.getMinRoadWidth(), 0) + " - " + global::formatFloat(course.getMaxRoadWidth(), 0); });
	panel.row("Road length", [this]() { return global::formatFloat(getEliteAgent()->getCourse()->getPathLength(), 0); });

	// Swatches double as the legend for car colours
	panel.header("Outcomes");
	panel.row("Alive", [this]() { return std::to_string(stats.alive) + " / " + std::to_string(stats.total); }, &theme::CAR_ALIVE);
	panel.row("Crashed", [this]() { return std::to_string(stats.crashed); }, &theme::CAR_CRASHED);
	panel.row("Timed out", [this]() { return std::to_string(stats.timedOut); }, &theme::CAR_TIMED_OUT);
	panel.row("Reached all", [this]() { return std::to_string(stats.completed); }, &theme::CAR_COMPLETED);
	panel.row("Furthest target", [this]() { return std::to_string(stats.furthestTarget); });
}

void NNDriverScenario::updateDisplay()
{
	Scenario::updateDisplay();

	if (!genepool->getGenepoolInitialized()) return;

	this->updateStats();
	this->updateTrail();
	this->updateEffects();
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
	const NNDriverAgent& elite = *getEliteAgent();
	if (trail.follow(getEliteIndex(), genepool->getGenerationNumber(), elite.getPosition())) lastHits = elite.getCurrentTarget();
}

void NNDriverScenario::updateEffects()
{
	effects.update(TIME_STEP);

	const NNDriverAgent& elite = *getEliteAgent();
	const DriverCourse& course = *elite.getCourse();

	// Dust kicked up behind the car, thicker the faster it goes
	const float speed = elite.getSpeed();
	if (!elite.getFinished() && speed > 60.0f)
	{
		const sf::Vector2f heading = elite.getHeading();
		const sf::Vector2f sideways(-heading.y, heading.x);
		const sf::Vector2f pos = elite.getPosition() - heading * 20.0f + sideways * randomRange(-8.0f, 8.0f);
		const sf::Vector2f vel = -heading * speed * 0.08f + sideways * randomRange(-20.0f, 20.0f);
		effects.emit(pos, vel, sf::Color(210, 195, 150), randomRange(0.4f, 0.8f), randomRange(3.0f, 6.0f) * (0.5f + speed / MAX_DRIVING_SPEED));
	}

	// Ripple out of a target whenever the elite reaches it
	const size_t hits = elite.getCurrentTarget();
	if (hits > lastHits && hits <= course.getTargetCount()) effects.ripple(course.getTarget(hits - 1), course.getTargetRadius(), theme::INFO);
	lastHits = hits;

	// One burst when the elite hits a wall
	if (!elite.getFinished()) crashDone = false;
	else if (!crashDone)
	{
		if (elite.getFinishType() == 0) this->spawnCrash(elite);
		crashDone = true;
	}
}

void NNDriverScenario::spawnCrash(const NNDriverAgent& elite)
{
	for (int i = 0; i < 60; i++)
	{
		const float angle = randomRange(0.0f, 2.0f * PI);
		const sf::Vector2f vel = sf::Vector2f(std::cos(angle), std::sin(angle)) * randomRange(40.0f, 240.0f);
		const sf::Color colour = i % 3 == 0 ? theme::NEGATIVE : sf::Color(255, 170, 80);
		effects.emit(elite.getPosition(), vel, colour, randomRange(0.5f, 1.3f), randomRange(1.5f, 3.5f), 80.0f);
	}
}

void NNDriverScenario::rebuildWalls(const DriverCourse& course)
{
	// Rebuild the wall shapes only when the course changes
	if (wallsNumber == course.getNumber()) return;

	walls.clear();
	for (const Body& body : course.getWalls())
	{
		sf::RectangleShape shape(body.size);
		shape.setFillColor(sf::Color(139, 94, 60));
		shape.setOutlineColor(sf::Color(78, 50, 28));
		shape.setOutlineThickness(3.0f);
		shape.setOrigin({ body.size.x / 2.0f, body.size.y / 2.0f });
		body.updateShape(shape);
		walls.push_back(shape);
	}
	wallsNumber = course.getNumber();
}

void NNDriverScenario::renderWorld(sf::RenderWindow* window)
{
	const DriverCourse& course = *getEliteAgent()->getCourse();
	this->rebuildWalls(course);

	this->renderGrass(window);
	this->renderRoad(window, course);
	this->renderWalls(window);
	this->renderTargets(window, *getEliteAgent());
}

void NNDriverScenario::renderGrass(sf::RenderWindow* window) const
{
	// Grass with mowed stripes, covering whatever the camera can see
	const sf::View& view = window->getView();
	const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
	theme::drawRect(window, topLeft, view.getSize(), sf::Color(66, 128, 62));
	for (float x = std::floor(topLeft.x / 200.0f) * 200.0f; x < topLeft.x + view.getSize().x; x += 200.0f)
	{
		theme::drawRect(window, { x, topLeft.y }, { 100.0f, view.getSize().y }, sf::Color(0, 40, 0, 28));
	}
}

void NNDriverScenario::renderRoad(sf::RenderWindow* window, const DriverCourse& course) const
{
	// Tarmac strip between the two edges, with a dashed line down the middle
	const std::vector<sf::Vector2f>& left = course.getLeftEdge();
	const std::vector<sf::Vector2f>& right = course.getRightEdge();
	sf::VertexArray road(sf::PrimitiveType::TriangleStrip);
	for (size_t i = 0; i < left.size(); i++)
	{
		road.append({ left[i], sf::Color(62, 64, 70) });
		road.append({ right[i], sf::Color(62, 64, 70) });
	}
	window->draw(road);

	sf::VertexArray dashes(sf::PrimitiveType::Triangles);
	const std::vector<sf::Vector2f>& path = course.getPath();
	for (size_t i = 0; i + 1 < path.size(); i++)
	{
		const sf::Vector2f direction = (path[i + 1] - path[i]).normalized();
		const float length = (path[i + 1] - path[i]).length();
		for (float d = 20.0f; d + 24.0f < length; d += 56.0f)
		{
			theme::appendLine(dashes, path[i] + direction * d, path[i] + direction * (d + 24.0f), 3.0f, sf::Color(255, 255, 255, 70));
		}
	}
	window->draw(dashes);

	// Start line across the road behind the cars
	const sf::Vector2f start = course.getPath().front();
	const sf::Vector2f direction = (course.getPath()[1] - start).normalized();
	const sf::Vector2f across(-direction.y, direction.x);
	sf::VertexArray line(sf::PrimitiveType::Triangles);
	theme::appendLine(line, start - direction * 40.0f - across * (course.getHalfWidth(0)), start - direction * 40.0f + across * (course.getHalfWidth(0)), 4.0f, sf::Color(255, 255, 255, 120));
	window->draw(line);
}

void NNDriverScenario::renderWalls(sf::RenderWindow* window) const
{
	// Soft shadows first so the walls sit above the grass
	for (const sf::RectangleShape& wall : walls)
	{
		sf::RectangleShape shadow = wall;
		shadow.move({ 6.0f, 8.0f });
		shadow.setFillColor(sf::Color(0, 0, 0, 60));
		shadow.setOutlineColor(sf::Color::Transparent);
		window->draw(shadow);
	}
	for (const sf::RectangleShape& wall : walls) window->draw(wall);
}

void NNDriverScenario::renderTargets(sf::RenderWindow* window, const NNDriverAgent& elite) const
{
	const DriverCourse& course = *elite.getCourse();
	const size_t count = course.getTargetCount();
	const size_t current = elite.getCurrentTarget();
	const float radius = course.getTargetRadius();
	const float time = clock.getElapsedTime().asSeconds();

	// Targets: bright and pulsing for the one being chased, dim once the elite has passed them
	for (size_t i = 0; i < count; i++)
	{
		const sf::Vector2f pos = course.getTarget(i);
		const bool isCurrent = i == current;
		const bool passed = i < current;
		const sf::Color base = isCurrent ? theme::INFO : sf::Color(255, 255, 255);

		if (isCurrent)
		{
			const float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
			theme::drawRing(window, pos, radius * (1.3f + 0.2f * pulse), 2.0f, sf::Color(base.r, base.g, base.b, theme::toAlpha(40.0f + 50.0f * (1.0f - pulse))));
		}

		theme::drawRing(window, pos, radius, 1.5f, sf::Color(base.r, base.g, base.b, isCurrent ? 230 : passed ? 50 : 110), sf::Color(base.r, base.g, base.b, isCurrent ? 56 : passed ? 10 : 24));
		theme::drawText(window, std::to_string(i + 1), pos, theme::TEXT_BODY, sf::Color(base.r, base.g, base.b, isCurrent ? 255 : passed ? 90 : 190), theme::Align::CENTER);
	}
}

void NNDriverScenario::renderAgents(sf::RenderWindow* window)
{
	if (getView() == AgentView::NONE) return;

	const NNDriverAgent* elite = getEliteAgent().get();

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

	if (!elite->getFinished() && elite->getCurrentTarget() < elite->getCourse()->getTargetCount())
	{
		const sf::Color lineColour(255, 255, 255, 90);
		const sf::Vertex line[2] = { { elite->getPosition(), lineColour }, { elite->getCourse()->getTarget(elite->getCurrentTarget()), lineColour } };
		window->draw(line, 2, sf::PrimitiveType::Lines);
	}
	elite->renderElite(window);
}

void NNDriverScenario::renderOverlay(sf::RenderWindow* window)
{
	const DriverGenepool::AgentPtr elite = getEliteAgent();
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

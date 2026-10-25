#include "stdafx.h"
#include "TbmlGlobal.h"
#include "CommonImpl.h"
#include "DogfightScenario.h"
#include "UIStatsPanel.h"
#include "UITheme.h"
#include "Utility.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr int LEVEL_PERIOD = 3;
	constexpr size_t HALL_SIZE = 12;

	float randomRange(float low, float high) { return low + (high - low) * tbml::fn::getRandomFloat(); }

	std::vector<NNInputSpec> makeInputSpecs()
	{
		std::vector<NNInputSpec> specs = { { "fwd v" }, { "side v" }, { "health" }, { "gun" }, { "boost" }, { "shield" }, { "shield t" } };
		for (size_t i = 0; i < DogfightPilot::RAY_COUNT; i++) specs.push_back({ "ray " + std::to_string(i * 360 / DogfightPilot::RAY_COUNT) });
		for (size_t rock = 1; rock <= DogfightPilot::NEAREST_ROCKS; rock++)
		{
			for (const char* label : { "x", "y", "vx", "vy" }) specs.push_back({ "rock" + std::to_string(rock) + " " + label });
		}
		for (const char* label : { "hit t", "hit dir", "foe dir", "foe dist", "foe aim", "foe lead", "foe hp", "shot t", "shot dir" }) specs.push_back({ label });
		return specs;
	}
}

DogfightScenario::DogfightScenario()
	: nnRenderer(makeInputSpecs(), { "thrust", "turn", "brake", "shoot", "boost", "shield" })
{
	this->genepool = std::make_shared<DogfightGenepool>([]()
	{
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(DogfightPilot::INPUT_COUNT, 20),
			std::make_shared<tbml::nn::Layer::TanH>(),
			std::make_shared<tbml::nn::Layer::Dense>(20, 12),
			std::make_shared<tbml::nn::Layer::TanH>(),
			std::make_shared<tbml::nn::Layer::Dense>(12, DogfightPilot::OUTPUT_COUNT),
			std::make_shared<tbml::nn::Layer::TanH>() }));
	},
		nullptr);

	// Every agent in a generation flies the same level against the same rival, a past champion picked from the hall of fame
	this->genepool->setCreateAgentFn([this](DogfightGenepool::GenomeCPtr data)
	{
		const int generation = this->genepool->getGenerationNumber();
		this->updateHall(generation);

		const Champion* rival = this->pickRival(generation);
		return std::make_unique<DogfightAgent>(std::move(data), this->getLevel(generation / LEVEL_PERIOD + 1), rival != nullptr ? rival->genome : nullptr, rival != nullptr ? rival->generation : 0);
	});

	this->genepool->configSelection(tbml::ga::SelectionType::ROULETTE);
	this->genepool->configThreading(false, true, false);
	this->genepool->resetGenepool(500, 0.06f);
	this->genepool->logInformation();

	for (int i = 0; i < 1100; i++)
	{
		stars.push_back({ { randomRange(0.0f, DogfightLevel::WIDTH), randomRange(0.0f, DogfightLevel::HEIGHT) }, randomRange(1.5f, 5.0f), randomRange(0.0f, 6.28f) });
	}
}

float DogfightScenario::getGenerationDifficulty(int generation) const
{
	return DogfightLevel::getGenerationDifficulty((generation - 1) / LEVEL_PERIOD + 1);
}

std::shared_ptr<const DogfightLevel> DogfightScenario::getLevel(int number)
{
	if (levelCache == nullptr || levelCache->getNumber() != number) levelCache = std::make_shared<const DogfightLevel>(number);
	return levelCache;
}

void DogfightScenario::updateHall(int generation)
{
	// Going backwards in generations means the genepool was reset
	if (generation < hallGeneration) hall.clear();
	hallGeneration = generation;

	// The best genome of the generation just finished joins the hall, the oldest drops out
	const NNGenome::GenomeCPtr best = genepool->getBestGenome();
	if (best == nullptr || (!hall.empty() && hall.back().genome == best)) return;

	hall.push_back({ best, generation });
	if (hall.size() > HALL_SIZE) hall.erase(hall.begin());
}

const DogfightScenario::Champion* DogfightScenario::pickRival(int generation) const
{
	// Mostly the latest champion, with every third generation facing someone older so nothing gets forgotten
	if (hall.empty()) return nullptr;

	size_t pick = hall.size() - 1;
	if (generation % 3 == 0) pick = static_cast<size_t>(generation / 3) % hall.size();
	return &hall[pick];
}

void DogfightScenario::initAgentStats(UIStatsPanel& panel)
{
	panel.row("Time", [this]() { return global::formatFloat(static_cast<float>(getEliteAgent()->getStep()) * DogfightArena::TIME_STEP, 1) + " / " + global::formatFloat(static_cast<float>(DogfightAgent::MAX_STEPS) * DogfightArena::TIME_STEP, 0) + " s"; });
	panel.row("Health", [this]() { return global::formatFloat(std::max(0.0f, getEliteAgent()->getArena().getShip(0).getHealth()), 0) + " / " + global::formatFloat(DogfightShip::MAX_HEALTH, 0); });
	panel.row("Rival health", [this]() { return global::formatFloat(std::max(0.0f, getEliteAgent()->getArena().getShip(1).getHealth()), 0) + " / " + global::formatFloat(DogfightShip::MAX_HEALTH, 0); });
	panel.row("Rocks shot", [this]() { return std::to_string(getEliteAgent()->getArena().getTally(0).rocks); });
	panel.row("Accuracy", [this]() { const DogfightArena::Tally& tally = getEliteAgent()->getArena().getTally(0); return global::formatFloat(100.0f * static_cast<float>(tally.hits) / static_cast<float>(std::max(1, tally.shots)), 0) + "%"; });
	panel.row("Wave", [this]() { return std::to_string(getEliteAgent()->getArena().getWave()); });

	panel.header("Level");
	panel.row("Number", [this]() { return std::to_string(getEliteAgent()->getLevel().getNumber()); });
	panel.row("Difficulty", [this]() { return global::formatFloat(getEliteAgent()->getLevel().getGenerationDifficulty() * 100.0f, 0) + "%"; });
	panel.row("Rocks", [this]() { return std::to_string(getEliteAgent()->getLevel().getAsteroidCount()) + " at " + global::formatFloat(getEliteAgent()->getLevel().getAsteroidSpeed(), 0); });
	panel.row("Boost", [this]() { return getEliteAgent()->getLevel().isBoostEnabled() ? std::string("On") : "Level " + std::to_string(DogfightLevel::BOOST_LEVEL); });
	panel.row("Shield", [this]() { return getEliteAgent()->getLevel().isShieldEnabled() ? std::string("On") : "Level " + std::to_string(DogfightLevel::SHIELD_LEVEL); });
	panel.row("Rival", [this]() { return getEliteAgent()->hasRival() ? "Gen " + std::to_string(getEliteAgent()->getRivalGeneration()) : std::string("Dummy"); }, &theme::DOGFIGHT_RIVAL);

	// Swatches double as the legend for ship colours
	panel.header("Outcomes");
	panel.row("Fighting", [this]() { return std::to_string(stats.fighting) + " / " + std::to_string(stats.total); }, &theme::DOGFIGHT_FIGHTING);
	panel.row("Won", [this]() { return std::to_string(stats.won); }, &theme::DOGFIGHT_WON);
	panel.row("Lost", [this]() { return std::to_string(stats.lost); }, &theme::DOGFIGHT_LOST);
	panel.row("Survived", [this]() { return std::to_string(stats.survived); }, &theme::DOGFIGHT_SURVIVED);
	panel.row("Win rate", [this]() { return global::formatFloat(100.0f * static_cast<float>(stats.won) / static_cast<float>(std::max<size_t>(1, stats.total)), 1) + "%"; });
}

void DogfightScenario::updateDisplay()
{
	Scenario::updateDisplay();

	if (!genepool->getGenepoolInitialized()) return;

	this->updateStats();
	this->updateParticles();
}

void DogfightScenario::updateStats()
{
	stats = PopulationStats();
	for (const auto& agent : *genepool->getAgentPopulation())
	{
		stats.total++;
		stats.step = std::max(stats.step, agent->getStep());

		switch (agent->getOutcome())
		{
		case DogfightOutcome::FIGHTING: stats.fighting++; break;
		case DogfightOutcome::WON: stats.won++; break;
		case DogfightOutcome::LOST: stats.lost++; break;
		case DogfightOutcome::SURVIVED: stats.survived++; break;
		}
	}
}

void DogfightScenario::updateParticles()
{
	effects.update(DogfightArena::TIME_STEP);

	const DogfightGenepool::AgentPtr elite = getEliteAgent();
	this->emitExhaust(*elite);
	this->emitEvents(*elite);
}

void DogfightScenario::emitExhaust(const DogfightAgent& elite)
{
	// Engine sparks trail off both ships
	for (size_t i = 0; i < 2; i++)
	{
		const DogfightShip& ship = elite.getArena().getShip(i);
		if (!ship.isAlive() || ship.getThrottle() < 0.1f) continue;

		const sf::Vector2f back = -ship.getHeading();
		const sf::Vector2f sideways(-back.y, back.x);
		const sf::Color colour = ship.isBoosting() ? sf::Color(150, 210, 255) : sf::Color(255, static_cast<std::uint8_t>(randomRange(130.0f, 200.0f)), 70);
		const sf::Vector2f vel = back * randomRange(80.0f, 180.0f) * (ship.isBoosting() ? 2.0f : 1.0f) + sideways * randomRange(-25.0f, 25.0f) + ship.getVelocity() * 0.5f;
		effects.emit(ship.getPosition() + back * 7.0f, vel, colour, randomRange(0.25f, 0.5f), randomRange(1.5f, 3.0f));
	}
}

void DogfightScenario::emitEvents(DogfightAgent& elite)
{
	for (const DogfightArena::Event& event : elite.takeEvents())
	{
		switch (event.type)
		{
		case DogfightArena::EventType::ROCK_DESTROYED:
			for (int i = 0; i < 5 + event.size * 4; i++)
			{
				const float angle = randomRange(0.0f, 2.0f * PI);
				const std::uint8_t grey = static_cast<std::uint8_t>(randomRange(150.0f, 230.0f));
				effects.emit(event.pos, sf::Vector2f(std::cos(angle), std::sin(angle)) * randomRange(30.0f, 40.0f + 30.0f * static_cast<float>(event.size)), sf::Color(grey, grey, static_cast<std::uint8_t>(grey + 20)), randomRange(0.4f, 0.9f), randomRange(1.5f, 3.5f));
			}
			break;

		case DogfightArena::EventType::SHIP_HIT:
			for (int i = 0; i < 12; i++)
			{
				const float angle = randomRange(0.0f, 2.0f * PI);
				effects.emit(event.pos, sf::Vector2f(std::cos(angle), std::sin(angle)) * randomRange(40.0f, 200.0f), i % 3 == 0 ? theme::NEGATIVE : sf::Color(255, 190, 90), randomRange(0.2f, 0.6f), randomRange(1.5f, 3.0f));
			}
			break;

		case DogfightArena::EventType::SHIP_BLOCKED:
			effects.ripple(event.pos, 10.0f, theme::INFO);
			break;

		case DogfightArena::EventType::SHIP_DESTROYED:
			for (int i = 0; i < 80; i++)
			{
				const float angle = randomRange(0.0f, 2.0f * PI);
				effects.emit(event.pos, sf::Vector2f(std::cos(angle), std::sin(angle)) * randomRange(30.0f, 320.0f), i % 3 == 0 ? theme::NEGATIVE : (i % 3 == 1 ? sf::Color(255, 170, 80) : theme::TEXT_PRIMARY), randomRange(0.6f, 1.6f), randomRange(1.5f, 4.0f));
			}
			effects.ripple(event.pos, 20.0f, theme::NEGATIVE);
			break;

		case DogfightArena::EventType::BOOST_STARTED:
			for (int i = 0; i < 16; i++)
			{
				const float angle = randomRange(0.0f, 2.0f * PI);
				effects.emit(event.pos, sf::Vector2f(std::cos(angle), std::sin(angle)) * randomRange(50.0f, 160.0f), sf::Color(150, 210, 255), randomRange(0.2f, 0.5f), randomRange(1.5f, 3.0f));
			}
			break;

		case DogfightArena::EventType::SHIELD_STARTED:
			effects.ripple(event.pos, 20.0f, theme::INFO);
			break;
		}
	}
}

void DogfightScenario::renderWorld(sf::RenderWindow* window)
{
	this->renderSpace(window);
}

void DogfightScenario::renderSpace(sf::RenderWindow* window) const
{
	// Deep space gradient across whatever the camera can see, a couple of soft nebulae, then twinkling stars
	const sf::View& view = window->getView();
	const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
	const sf::Vector2f size = view.getSize();
	const sf::Color top(6, 8, 22);
	const sf::Color bottom(22, 16, 42);

	sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
	sky.append({ topLeft, top });
	sky.append({ { topLeft.x + size.x, topLeft.y }, top });
	sky.append({ { topLeft.x, topLeft.y + size.y }, bottom });
	sky.append({ topLeft + size, bottom });
	window->draw(sky);

	sf::VertexArray nebulae(sf::PrimitiveType::Triangles);
	theme::appendDisc(nebulae, { 750.0f, 620.0f }, 850.0f, sf::Color(70, 40, 120, 14), 40);
	theme::appendDisc(nebulae, { 750.0f, 620.0f }, 500.0f, sf::Color(90, 50, 150, 14), 40);
	theme::appendDisc(nebulae, { 2900.0f, 1900.0f }, 950.0f, sf::Color(30, 80, 140, 14), 40);
	theme::appendDisc(nebulae, { 2900.0f, 1900.0f }, 550.0f, sf::Color(40, 100, 170, 14), 40);
	window->draw(nebulae);

	const float time = clock.getElapsedTime().asSeconds();
	sf::VertexArray starVertices(sf::PrimitiveType::Triangles);
	for (const Star& star : stars)
	{
		const float twinkle = 0.55f + 0.45f * std::sin(time * 1.5f + star.phase);
		theme::appendDisc(starVertices, star.pos, star.size * 0.5f, sf::Color(220, 230, 255, theme::toAlpha(200.0f * twinkle)), 6);
	}
	window->draw(starVertices);

	// Faint frame marking the edge of the wrapped arena
	sf::VertexArray frame(sf::PrimitiveType::Triangles);
	const sf::Color edge(255, 255, 255, 26);
	theme::appendLine(frame, { 0.0f, 0.0f }, { DogfightLevel::WIDTH, 0.0f }, 6.0f, edge);
	theme::appendLine(frame, { DogfightLevel::WIDTH, 0.0f }, { DogfightLevel::WIDTH, DogfightLevel::HEIGHT }, 6.0f, edge);
	theme::appendLine(frame, { DogfightLevel::WIDTH, DogfightLevel::HEIGHT }, { 0.0f, DogfightLevel::HEIGHT }, 6.0f, edge);
	theme::appendLine(frame, { 0.0f, DogfightLevel::HEIGHT }, { 0.0f, 0.0f }, 6.0f, edge);
	window->draw(frame);
}

void DogfightScenario::renderAgents(sf::RenderWindow* window)
{
	if (getView() == AgentView::NONE) return;

	const DogfightAgent* elite = getEliteAgent().get();

	// Whole pack as ghost ships in one draw call, the elite's rocks, bullets and rival are drawn on top
	if (getView() == AgentView::ALL)
	{
		packVertices.clear();
		for (const auto& agent : *genepool->getAgentPopulation())
		{
			if (agent.get() != elite) agent->appendPackVertices(packVertices);
		}
		window->draw(packVertices);
	}

	elite->renderElite(window);
	effects.render(window);
}

void DogfightScenario::renderOverlay(sf::RenderWindow* window)
{
	const DogfightGenepool::AgentPtr elite = getEliteAgent();
	nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

#include "stdafx.h"
#include "AppScenarioController.h"
#include "UIIconButton.h"
#include "UILabel.h"
#include "UIPanel.h"
#include "UIStatsPanel.h"
#include "UITooltip.h"

AppScenarioController::AppScenarioController(ScenarioUPtr&& scenario, sf::RenderWindow* window)
	: scenario(std::move(scenario))
{
	this->genepool = this->scenario->getGenepool();
	this->initUI(window);
}

void AppScenarioController::initUI(sf::RenderWindow* window)
{
	this->ui = std::make_unique<UIManager>();

	const float tile = 26.0f;
	const float tileGap = 2.0f;
	const float pad = theme::PADDING;
	const sf::Vector2f origin(theme::MARGIN, theme::MARGIN);

	// Controls panel: icon tiles with each auto toggle directly beside the one-shot it repeats, then a speed stepper
	const float controlsHeight = pad + tile + theme::GAP + tile + pad;
	this->ui->addElement(std::make_shared<UIPanel>(origin, sf::Vector2f{ theme::PANEL_WIDTH, controlsHeight }));

	float x = origin.x + pad;
	auto addIcon = [&](float gapBefore, Icon icon, Icon activeIcon, const std::string& tooltip, bool isToggle, std::function<void(bool)> action)
	{
		x += gapBefore;
		this->ui->addElement(std::make_shared<UIIconButton>(window, sf::Vector2f{ x, origin.y + pad }, sf::Vector2f{ tile, tile }, icon, activeIcon, tooltip, isToggle, std::move(action)));
		x += tile;
	};

	addIcon(0.0f, Icon::PLAY, Icon::PAUSE, "Run / pause evaluation", true, [&](bool toggled) { this->setEvaluating(toggled); });
	addIcon(theme::GAP, Icon::RUN_TO_END, Icon::RUN_TO_END, "Evaluate this generation now", false, [&](bool) { this->fullEvaluate(); });
	addIcon(tileGap, Icon::LOOP, Icon::LOOP, "Auto: evaluate whole generations", true, [&](bool toggled) { this->setFullEvaluating(toggled); });
	addIcon(theme::GAP, Icon::NEXT, Icon::NEXT, "Next generation", false, [&](bool) { this->iterate(); });
	addIcon(tileGap, Icon::LOOP, Icon::LOOP, "Auto: advance generations", true, [&](bool toggled) { this->setIterating(toggled); });
	addIcon(theme::GAP, Icon::FAST, Icon::FAST, "Fast mode: run generations back to back with the scenario hidden", true, [&](bool toggled) { this->setFastMode(toggled); });

	// Speed stepper: [-] 1x [+] with minimal gaps
	const float stepperTop = origin.y + pad + tile + theme::GAP;
	const float readoutWidth = 44.0f;
	this->ui->addElement(std::make_shared<UIIconButton>(window, sf::Vector2f{ origin.x + pad, stepperTop }, sf::Vector2f{ tile, tile }, Icon::MINUS, Icon::MINUS,
		"Slower", false, [&](bool) { this->changeSpeed(-1); }));
	this->ui->addElement(std::make_shared<UILabel>(sf::Vector2f{ origin.x + pad + tile + tileGap + readoutWidth / 2.0f, stepperTop + tile / 2.0f },
		[&]() { return this->getSpeedText(); }, theme::TEXT_BODY, theme::TEXT_PRIMARY, theme::Align::CENTER));
	this->ui->addElement(std::make_shared<UIIconButton>(window, sf::Vector2f{ origin.x + pad + tile + tileGap + readoutWidth + tileGap, stepperTop }, sf::Vector2f{ tile, tile }, Icon::PLUS, Icon::PLUS,
		"Faster", false, [&](bool) { this->changeSpeed(1); }));

	// Stats panel sits below the controls
	UIStatsPanel stats(window, this->ui.get(), { origin.x, origin.y + controlsHeight + theme::GAP }, theme::PANEL_WIDTH);

	stats.header("Network");
	stats.row("Generation", [&]() { return std::to_string(this->genepool->getGenerationNumber()); });
	stats.row("Population", [&]() { return std::to_string(this->genepool->getPopulationSize()); });
	stats.row("Evaluated", [&]() { return std::string(this->genepool->getGenerationEvaluated() ? "Yes" : "No"); });
	stats.row("Best fitness", [&]() { return global::formatFloat(this->genepool->getBestFitness()); });

	stats.header("Simulation");
	stats.row("FPS", [&]() { return global::formatFloat(this->fps, 0); });
	stats.row("Speed", [&]() { return this->getSpeedText(); });

	this->scenario->initStats(stats);
	stats.finish();

	// Added last so it draws over everything
	this->ui->addElement(std::make_shared<UITooltip>());
}

std::string AppScenarioController::getSpeedText() const
{
	std::ostringstream ss;
	ss << SPEEDS[speedIndex] << "x";
	return ss.str();
}

void AppScenarioController::handleEvent(const sf::Event& event)
{
	scenario->handleEvent(event);
}

void AppScenarioController::update()
{
	if (!genepool->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");

	const float dt = frameClock.restart().asSeconds();
	if (dt > 0.0f) fps += (1.0f / dt - fps) * 0.1f;

	this->ui->update();

	if (isFastMode)
	{
		this->runFast();
		return;
	}

	scenario->updateDisplay();

	// Speeds below 1 accumulate until a whole step is due; full evaluation already runs a whole generation, so it caps at 1
	stepAccumulator += SPEEDS[speedIndex];
	int steps = static_cast<int>(stepAccumulator);
	stepAccumulator -= static_cast<float>(steps);
	if (toFullEvaluate) steps = std::min(steps, 1);

	for (int i = 0; i < steps; i++)
	{
		if (!genepool->getGenerationEvaluated() && (evaluating || toFullEvaluate)) genepool->evaluateGeneration(!toFullEvaluate);

		if (genepool->getGenerationEvaluated() && iterating) genepool->iterateGeneration();
	}
}

void AppScenarioController::render(sf::RenderWindow* window)
{
	if (!isFastMode)
	{
		if (!genepool->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");

		scenario->render(window);
	}

	window->setView(theme::screenView(window));
	this->ui->render(window);
}

void AppScenarioController::fullEvaluate()
{
	if (!genepool->getGenepoolInitialized()) return;

	if (genepool->getGenerationEvaluated()) return;

	genepool->evaluateGeneration(false);
}

void AppScenarioController::iterate()
{
	if (!genepool->getGenepoolInitialized()) return;

	if (!genepool->getGenerationEvaluated()) return;

	genepool->iterateGeneration();
}

void AppScenarioController::runFast()
{
	// Evaluate and advance whole generations until the frame budget is spent, leaving the UI responsive
	const sf::Clock clock;
	do
	{
		if (!genepool->getGenerationEvaluated()) genepool->evaluateGeneration(false);
		genepool->iterateGeneration();
	}
	while (clock.getElapsedTime().asMilliseconds() < FAST_FRAME_MS);
}

void AppScenarioController::changeSpeed(int direction)
{
	const int count = static_cast<int>(std::size(SPEEDS));
	speedIndex = static_cast<size_t>(std::clamp(static_cast<int>(speedIndex) + direction, 0, count - 1));
}

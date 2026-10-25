#include "stdafx.h"
#include "Scenario.h"
#include "UIStatsPanel.h"

void Scenario::initStats(UIStatsPanel& panel)
{
	panel.header("Agents");
	panel.radio({ "All", "Elite", "None" },
		[this]() { return static_cast<size_t>(view); },
		[this](size_t index) { view = static_cast<AgentView>(index); });
	panel.row("Elite fitness", [this]() { return global::formatFloat(getGenepool()->getAgentFitness(eliteIndex)); });
	panel.row("Average fitness", [this]() { return global::formatFloat(averageFitness); });

	this->initAgentStats(panel);
}

void Scenario::updateDisplay()
{
	if (!getGenepool()->getGenepoolInitialized()) return;

	this->updateElite();
}

void Scenario::handleEvent(const sf::Event& event)
{
	camera.handleEvent(event);
}

void Scenario::render(sf::RenderWindow* window)
{
	if (!camera.apply(window, getWorldSize())) return;

	this->renderWorld(window);
	this->renderAgents(window);

	// Panels are drawn in screen space
	window->setView(theme::screenView(window));
	std::function<float(int)> difficulty;
	if (this->hasDifficulty()) difficulty = [this](int generation) { return this->getGenerationDifficulty(generation); };
	graphs.render(window, *getGenepool(), difficulty);
	this->renderOverlay(window);

	const sf::Vector2f size(window->getSize());
	theme::drawText(window, "Scroll to zoom, right drag to pan, middle click to reset", { size.x / 2.0f, size.y - theme::MARGIN - 6.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
}

void Scenario::updateElite()
{
	const tbml::ga::IGenepoolPtr genepool = getGenepool();
	const size_t count = genepool->getPopulationSize();
	if (count == 0) return;

	// Find the fittest agent and the average
	size_t best = 0;
	float total = 0.0f;
	for (size_t i = 0; i < count; i++)
	{
		const float fitness = genepool->getAgentFitness(i);
		total += fitness;
		if (fitness > genepool->getAgentFitness(best)) best = i;
	}
	averageFitness = total / static_cast<float>(count);

	// After the first generation the elite is last generation's best, carried over by elitism into the first slot
	const int generation = genepool->getGenerationNumber();
	if (generation > 1)
	{
		eliteIndex = 0;
		eliteGeneration = generation;
		return;
	}

	// Only swap elite on a clear lead (or a new generation) so the highlight doesn't flicker between near ties
	if (generation != eliteGeneration || genepool->getAgentFitness(best) > genepool->getAgentFitness(eliteIndex) * 1.03f + 0.1f)
	{
		eliteIndex = best;
		eliteGeneration = generation;
	}
}

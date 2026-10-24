#pragma once

#include <deque>
#include "NNDriverGenepool.h"
#include "NNRenderer.h"
#include "Scenario.h"

class NNDriverScenario : public Scenario
{
public:
	NNDriverScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }
	void updateDisplay() override;

protected:
	void initAgentStats(UIStatsPanel& panel) override;
	void renderWorld(sf::RenderWindow* window) override;
	void renderAgents(sf::RenderWindow* window) override;
	void renderOverlay(sf::RenderWindow* window) override;

private:
	struct PopulationStats
	{
		size_t total = 0;
		size_t alive = 0;
		size_t crashed = 0;
		size_t timedOut = 0;
		size_t completed = 0;
		size_t furthestTarget = 0;
		size_t step = 0;
	};

	NNDriverGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	void updateStats();
	void updateTrail();
	void renderTrail(sf::RenderWindow* window);

	std::shared_ptr<NNDriverGenepool> genepool = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	std::deque<sf::Vector2f> trail;
	size_t trailIndex = 0;
	int trailGeneration = -1;
};

#pragma once

#include "NNPoleBalancerAgent.h"
#include "NNRenderer.h"
#include "Scenario.h"
#include "Trail.h"

class NNPoleBalancerScenario : public Scenario
{
public:
	NNPoleBalancerScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }
	void updateDisplay() override;

protected:
	bool hasDifficulty() const override { return true; }
	float getDifficulty(int generation) const override;
	void initAgentStats(UIStatsPanel& panel) override;
	void renderWorld(sf::RenderWindow* window) override;
	void renderAgents(sf::RenderWindow* window) override;
	void renderOverlay(sf::RenderWindow* window) override;

private:
	using PoleGenepool = tbml::ga::Genepool<NNGenome, NNPoleBalancerAgent>;

	struct PopulationStats
	{
		size_t total = 0;
		size_t balancing = 0;
		size_t fell = 0;
		size_t offTrack = 0;
		size_t survived = 0;
		size_t step = 0;
	};

	PoleGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	void updateStats();
	void updateTrail();
	void renderBackdrop(sf::RenderWindow* window) const;
	void renderTrack(sf::RenderWindow* window) const;
	void renderWedge(sf::RenderWindow* window, const NNPoleBalancerAgent& elite) const;

	std::shared_ptr<PoleGenepool> genepool = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	Trail trail = Trail(90, 3.0f, 120);
	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
};

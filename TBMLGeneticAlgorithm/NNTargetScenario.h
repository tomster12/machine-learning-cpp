#pragma once

#include "NNRenderer.h"
#include "NNTargetAgent.h"
#include "ParticleSystem.h"
#include "Scenario.h"
#include "Trail.h"

class NNTargetScenario : public Scenario
{
public:
	NNTargetScenario();
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
	using TargetGenepool = tbml::ga::Genepool<NNGenome, NNTargetAgent>;

	struct PopulationStats
	{
		size_t total = 0;
		size_t running = 0;
		size_t furthestTarget = 0;
		size_t step = 0;
	};

	TargetGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	std::shared_ptr<const NNTargetAgent::Targets> getTargets(int number);
	void updateStats();
	void updateTrail();
	void updateEffects();
	void renderBackdrop(sf::RenderWindow* window) const;
	void renderTargets(sf::RenderWindow* window, const NNTargetAgent& elite) const;

	std::shared_ptr<TargetGenepool> genepool = nullptr;
	std::shared_ptr<const NNTargetAgent::Targets> targetsCache = nullptr;
	int targetsNumber = -1;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	Trail trail = Trail(240, 3.0f);
	ParticleSystem effects;
	size_t lastHits = 0;
	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
};

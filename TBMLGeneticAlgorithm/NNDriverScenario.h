#pragma once

#include "NNDriverAgent.h"
#include "NNRenderer.h"
#include "ParticleSystem.h"
#include "Scenario.h"
#include "Trail.h"

class NNDriverScenario : public Scenario
{
public:
	NNDriverScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }
	void updateDisplay() override;

protected:
	bool hasDifficulty() const override { return true; }
	float getDifficulty(int generation) const override;
	sf::Vector2f getWorldSize() const override { return getEliteAgent()->getCourse()->getSize(); }
	void initAgentStats(UIStatsPanel& panel) override;
	void renderWorld(sf::RenderWindow* window) override;
	void renderAgents(sf::RenderWindow* window) override;
	void renderOverlay(sf::RenderWindow* window) override;

private:
	using DriverGenepool = tbml::ga::Genepool<NNGenome, NNDriverAgent>;

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

	DriverGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	std::shared_ptr<const DriverCourse> getCourse(int number);
	void updateStats();
	void updateTrail();
	void updateEffects();
	void spawnCrash(const NNDriverAgent& elite);
	void rebuildWalls(const DriverCourse& course);
	void renderGrass(sf::RenderWindow* window) const;
	void renderRoad(sf::RenderWindow* window, const DriverCourse& course) const;
	void renderWalls(sf::RenderWindow* window) const;
	void renderTargets(sf::RenderWindow* window, const NNDriverAgent& elite) const;

	std::shared_ptr<DriverGenepool> genepool = nullptr;
	std::shared_ptr<const DriverCourse> courseCache = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	int wallsNumber = -1;
	std::vector<sf::RectangleShape> walls;
	Trail trail = Trail(200, 3.0f);
	ParticleSystem effects;
	size_t lastHits = 0;
	bool crashDone = false;
	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
};

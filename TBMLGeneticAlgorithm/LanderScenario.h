#pragma once

#include "LanderAgent.h"
#include "NNRenderer.h"
#include "ParticleSystem.h"
#include "Scenario.h"
#include "Trail.h"

class LanderScenario : public Scenario
{
public:
	LanderScenario();
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
	using LanderGenepool = tbml::ga::Genepool<NNGenome, LanderAgent>;

	struct PopulationStats
	{
		size_t total = 0;
		size_t flying = 0;
		size_t landed = 0;
		size_t crashed = 0;
		size_t lost = 0;
		size_t step = 0;
	};

	struct Star
	{
		sf::Vector2f pos;
		float size = 1.0f;
		float phase = 0.0f;
	};

	LanderGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	std::shared_ptr<const LanderCourse> getCourse(int number);
	void updateStats();
	void updateTrail();
	void updateParticles();
	void spawnBurst(const LanderAgent& elite);
	void renderSky(sf::RenderWindow* window) const;
	void renderTerrain(sf::RenderWindow* window, const LanderCourse& course);
	void renderPad(sf::RenderWindow* window, const LanderCourse& course) const;

	std::shared_ptr<LanderGenepool> genepool = nullptr;
	std::shared_ptr<const LanderCourse> courseCache = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	std::vector<Star> stars;
	Trail trail = Trail(300, 4.0f);
	ParticleSystem effects;
	bool burstDone = false;

	int terrainNumber = -1;
	sf::VertexArray terrainFill = sf::VertexArray(sf::PrimitiveType::TriangleStrip);
	sf::VertexArray terrainEdge = sf::VertexArray(sf::PrimitiveType::LineStrip);
	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
};

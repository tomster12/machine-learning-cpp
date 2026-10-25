#pragma once

#include <deque>
#include "LanderAgent.h"
#include "NNRenderer.h"
#include "Scenario.h"

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

	struct Particle
	{
		sf::Vector2f pos;
		sf::Vector2f vel;
		sf::Color colour;
		float life = 0.0f;
		float maxLife = 1.0f;
		float size = 2.0f;
		float gravity = 0.0f;
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
	void renderTrail(sf::RenderWindow* window) const;
	void renderParticles(sf::RenderWindow* window) const;

	std::shared_ptr<LanderGenepool> genepool = nullptr;
	std::shared_ptr<const LanderCourse> courseCache = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	std::vector<Star> stars;
	std::deque<sf::Vector2f> trail;
	std::vector<Particle> particles;
	size_t trailIndex = 0;
	int trailGeneration = -1;
	bool burstDone = false;

	int terrainNumber = -1;
	sf::VertexArray terrainFill = sf::VertexArray(sf::PrimitiveType::TriangleStrip);
	sf::VertexArray terrainEdge = sf::VertexArray(sf::PrimitiveType::LineStrip);
	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
};

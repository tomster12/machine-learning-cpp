#pragma once

#include "NNRenderer.h"
#include "ParticleSystem.h"
#include "Scenario.h"
#include "Trail.h"
#include "WalkerAgent.h"

class WalkerScenario : public Scenario
{
public:
	WalkerScenario();
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
	using WalkerGenepool = tbml::ga::Genepool<NNGenome, WalkerAgent>;

	struct PopulationStats
	{
		size_t total = 0;
		size_t walking = 0;
		size_t fell = 0;
		size_t timedOut = 0;
		size_t finished = 0;
		size_t step = 0;
		float furthest = 0.0f;
	};

	WalkerGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	std::shared_ptr<const WalkerCourse> getCourse(int number);
	void updateStats();
	void updateParticles();
	void buildHills();
	void renderSky(sf::RenderWindow* window) const;
	void renderTerrain(sf::RenderWindow* window, const WalkerCourse& course);
	void renderMarkers(sf::RenderWindow* window, const WalkerCourse& course) const;

	std::shared_ptr<WalkerGenepool> genepool = nullptr;
	std::shared_ptr<const WalkerCourse> courseCache = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	Trail trail = Trail(300, 3.0f);
	ParticleSystem effects;
	std::array<bool, 2> lastContact = {};
	bool burstDone = false;

	int terrainNumber = -1;
	sf::VertexArray terrainFill = sf::VertexArray(sf::PrimitiveType::TriangleStrip);
	sf::VertexArray terrainEdge = sf::VertexArray(sf::PrimitiveType::LineStrip);
	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
	sf::VertexArray farHills = sf::VertexArray(sf::PrimitiveType::TriangleStrip);
	sf::VertexArray nearHills = sf::VertexArray(sf::PrimitiveType::TriangleStrip);
};

#pragma once

#include "DogfightAgent.h"
#include "NNRenderer.h"
#include "ParticleSystem.h"
#include "Scenario.h"

class DogfightScenario : public Scenario
{
public:
	DogfightScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }
	void updateDisplay() override;

protected:
	bool hasDifficulty() const override { return true; }
	sf::Vector2f getWorldSize() const override { return { DogfightLevel::WIDTH, DogfightLevel::HEIGHT }; }
	float getGenerationDifficulty(int generation) const override;
	void initAgentStats(UIStatsPanel& panel) override;
	void renderWorld(sf::RenderWindow* window) override;
	void renderAgents(sf::RenderWindow* window) override;
	void renderOverlay(sf::RenderWindow* window) override;

private:
	using DogfightGenepool = tbml::ga::Genepool<NNGenome, DogfightAgent>;

	struct PopulationStats
	{
		size_t total = 0;
		size_t fighting = 0;
		size_t won = 0;
		size_t lost = 0;
		size_t survived = 0;
		size_t step = 0;
	};

	struct Champion
	{
		NNGenome::GenomeCPtr genome;
		int generation = 0;
	};

	struct Star
	{
		sf::Vector2f pos;
		float size = 1.0f;
		float phase = 0.0f;
	};

	DogfightGenepool::AgentPtr getEliteAgent() const { return getElite(*genepool->getAgentPopulation()); }
	std::shared_ptr<const DogfightLevel> getLevel(int number);
	void updateHall(int generation);
	const Champion* pickRival(int generation) const;
	void updateStats();
	void updateParticles();
	void emitEvents(DogfightAgent& elite);
	void emitExhaust(const DogfightAgent& elite);
	void renderSpace(sf::RenderWindow* window) const;

	std::shared_ptr<DogfightGenepool> genepool = nullptr;
	std::shared_ptr<const DogfightLevel> levelCache = nullptr;
	NNRenderer nnRenderer;
	PopulationStats stats;
	sf::Clock clock;

	std::vector<Champion> hall;
	int hallGeneration = -1;
	std::vector<Star> stars;
	ParticleSystem effects;

	sf::VertexArray packVertices = sf::VertexArray(sf::PrimitiveType::Triangles);
};

#pragma once

#include <GenepoolSimulation.h>
#include "Camera.h"
#include "FitnessGraph.h"

class UIStatsPanel;

enum class AgentView { ALL, ELITE, NONE };

class Scenario
{
public:
	virtual ~Scenario() = default;
	virtual tbml::ga::IGenepoolPtr getGenepool() const = 0;
	virtual void updateDisplay();

	void initStats(UIStatsPanel& panel);
	void handleEvent(const sf::Event& event);
	void render(sf::RenderWindow* window);

protected:
	virtual sf::Vector2f getWorldSize() const { return { 1400.0f, 1000.0f }; };
	virtual void initAgentStats(UIStatsPanel& panel) {};
	virtual void renderWorld(sf::RenderWindow* window) {};
	virtual void renderAgents(sf::RenderWindow* window) = 0;
	virtual void renderOverlay(sf::RenderWindow* window) {};

	AgentView getView() const { return view; }
	size_t getEliteIndex() const { return eliteIndex; }

	template<class TAgentPtr>
	const TAgentPtr& getElite(const std::vector<TAgentPtr>& population) const { return population[eliteIndex]; }

	template<class TAgentPtr>
	void renderGenepoolAgents(sf::RenderWindow* window, const std::vector<TAgentPtr>& population) const
	{
		if (view == AgentView::NONE) return;

		// Draw the elite last so it sits on top
		if (view == AgentView::ALL)
		{
			for (size_t i = 0; i < population.size(); i++)
			{
				if (i != eliteIndex) population[i]->render(window, false);
			}
		}
		population[eliteIndex]->render(window, true);
	}

private:
	void updateElite();

	Camera camera;
	FitnessGraph graph;
	AgentView view = AgentView::ALL;
	size_t eliteIndex = 0;
	int eliteGeneration = -1;
	float averageFitness = 0.0f;
};

using ScenarioUPtr = std::unique_ptr<Scenario>;

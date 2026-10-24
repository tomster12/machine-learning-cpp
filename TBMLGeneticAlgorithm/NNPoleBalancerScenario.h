#pragma once

#include "NNPoleBalancerAgent.h"
#include "NNRenderer.h"
#include "Scenario.h"

class NNPoleBalancerScenario : public Scenario
{
public:
	NNPoleBalancerScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }

protected:
	void renderAgents(sf::RenderWindow* window) override;
	void renderOverlay(sf::RenderWindow* window) override;

private:
	std::shared_ptr<tbml::ga::Genepool<NNGenome, NNPoleBalancerAgent>> genepool = nullptr;
	NNRenderer nnRenderer;
};

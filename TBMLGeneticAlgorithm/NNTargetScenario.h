#pragma once

#include "NNRenderer.h"
#include "NNTargetGenepool.h"
#include "Scenario.h"

class NNTargetScenario : public Scenario
{
public:
	NNTargetScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }

protected:
	void renderWorld(sf::RenderWindow* window) override;
	void renderAgents(sf::RenderWindow* window) override;
	void renderOverlay(sf::RenderWindow* window) override;

private:
	std::shared_ptr<NNTargetGenepool> genepool = nullptr;
	NNRenderer nnRenderer;
};

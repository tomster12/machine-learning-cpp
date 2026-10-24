#pragma once

#include "Scenario.h"
#include "VectorListTargetGenepool.h"

class VectorListTargetScenario : public Scenario
{
public:
	VectorListTargetScenario();
	tbml::ga::IGenepoolPtr getGenepool() const override { return genepool; }

protected:
	void renderWorld(sf::RenderWindow* window) override;
	void renderAgents(sf::RenderWindow* window) override;

private:
	std::shared_ptr<VectorListTargetGenepool> genepool = nullptr;
};

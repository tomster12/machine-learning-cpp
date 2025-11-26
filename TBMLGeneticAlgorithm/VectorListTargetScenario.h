#pragma once

#include "AppScenarioController.h"
#include "VectorListTargetAgent.h"

class VectorListTargetScenario : public IAppScenario
{
public:
	VectorListTargetScenario();
	void render(sf::RenderWindow* window) override;
	tbml::ga::IGenepoolPtr getGenepool() const override { return this->genepool; };

private:
	std::shared_ptr<VectorListTargetGenepool> genepool = nullptr;
};

#pragma once

#include "GenepoolSimulation.h"
#include "IAppScenario.h"
#include "NNPoleBalancerAgent.h"

class NNPoleBalancerScenario : public IAppScenario
{
public:
	NNPoleBalancerScenario();
	void render(sf::RenderWindow* window) override;
	tbml::ga::IGenepoolPtr getGenepool() const override { return std::static_pointer_cast<tbml::ga::IGenepool>(genepool); }

private:
	std::shared_ptr<tbml::ga::Genepool<NNGenome, NNPoleBalancerAgent>> genepool = nullptr;
};

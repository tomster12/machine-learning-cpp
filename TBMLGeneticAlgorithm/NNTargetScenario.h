#pragma once

#include "GenepoolSimulation.h"
#include "IAppScenario.h"
#include "NNTargetGenepool.h"

class NNTargetScenario : public IAppScenario
{
public:
	NNTargetScenario();
	void render(sf::RenderWindow* window) override;
	tbml::ga::IGenepoolPtr getGenepool() const override { return std::static_pointer_cast<tbml::ga::IGenepool>(genepool); }

private:
	std::shared_ptr<NNTargetGenepool> genepool = nullptr;
};

#pragma once

#include "GenepoolSimulation.h"
#include "IAppScenario.h"
#include "NNDriverGenepool.h"

class NNDriverScenario : public IAppScenario
{
public:
	NNDriverScenario();
	void render(sf::RenderWindow* window) override;
	tbml::ga::IGenepoolPtr getGenepool() const override { return std::static_pointer_cast<tbml::ga::IGenepool>(genepool); }

private:
	std::shared_ptr<NNDriverGenepool> genepool = nullptr;
};

#pragma once

#include <GenepoolSimulation.h>

class FitnessGraph
{
public:
	void render(sf::RenderWindow* window, const std::vector<tbml::ga::GenerationStats>& history) const;
};

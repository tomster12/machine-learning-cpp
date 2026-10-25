#pragma once

#include <GenepoolSimulation.h>
#include "Icon.h"

class GraphDock
{
public:
	void render(sf::RenderWindow* window, tbml::ga::IGenepool& genepool, const std::function<float(int)>& difficulty);

private:
	enum class Tab { FITNESS, SPREAD, MUTATION, DIFFICULTY };

	void renderFitness(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const std::vector<tbml::ga::GenerationStats>& history) const;
	void renderSpread(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const tbml::ga::IGenepool& genepool);
	void renderMutation(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, tbml::ga::IGenepool& genepool, sf::Vector2f mouse, bool clicked) const;
	void renderDifficulty(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const tbml::ga::IGenepool& genepool, const std::function<float(int)>& difficulty) const;

	Tab tab = Tab::FITNESS;
	bool isPressed = false;
	std::vector<std::pair<float, bool>> ranked;
};

#pragma once

#include "CommonImpl.h"
#include "NNTargetAgent.h"

class NNTargetGenepool : public tbml::ga::Genepool<NNGenome, NNTargetAgent>
{
public:
	NNTargetGenepool(
		std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
		std::vector<sf::Vector2f> targets, float targetRadius);

	void initVisual();
	void render(sf::RenderWindow* window);
	const sf::Vector2f& getTarget(int index) const;
	size_t getTargetCount() const;
	float getTargetRadius() const;

protected:
	bool isVisualInit = false;
	std::vector<sf::CircleShape> targetShapes;
	std::vector<sf::Vector2f> targetPos;
	float targetRadius = 0.0f;
};

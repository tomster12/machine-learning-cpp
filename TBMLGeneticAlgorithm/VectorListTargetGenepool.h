#pragma once

#include "CommonImpl.h"
#include "VectorListTargetAgent.h"

class VectorListTargetGenepool : public tbml::ga::Genepool<VectorListGenome, VectorListTargetAgent>
{
public:
	VectorListTargetGenepool(
		std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
		sf::Vector2f targetPos, float targetRadius);

	void initVisual();
	void render(sf::RenderWindow* window) override;
	sf::Vector2f getTargetPos() const;
	float getTargetRadius() const;

protected:
	sf::CircleShape target;
	sf::Vector2f targetPos;
	float targetRadius = 0.0f;
};

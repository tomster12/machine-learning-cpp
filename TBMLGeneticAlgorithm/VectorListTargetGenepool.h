#pragma once

#include "CommonImpl.h"
#include "VectorListTargetAgent.h"
#include "AppGenepoolController.h"

class VectorListTargetGenepool : public tbml::ga::Genepool<VectorListGenome, VectorListTargetAgent>, public IAppGenepool
{
public:
	VectorListTargetGenepool(
		std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
		sf::Vector2f targetPos, float targetRadius);

	void initVisual();
	void render(sf::RenderWindow* window);
	sf::Vector2f getTargetPos() const;
	float getTargetRadius() const;

	static IAppGenepoolPtr createGenepool();

protected:
	sf::CircleShape target;
	sf::Vector2f targetPos;
	float targetRadius = 0.0f;
};

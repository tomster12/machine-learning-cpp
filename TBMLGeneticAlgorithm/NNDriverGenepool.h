#pragma once

#include "Body.h"
#include "CommonImpl.h"
#include "NNDriverAgent.h"

class NNDriverGenepool : public tbml::ga::Genepool<NNGenome, NNDriverAgent>
{
public:
	NNDriverGenepool(
		std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
		std::vector<sf::Vector2f> targets, float targetRadius, std::vector<Body> worldBodies);

	void initVisual();
	void render(sf::RenderWindow* window);
	bool checkWorldIntersectBody(Body& body) const;
	bool checkWorldIntersectRaycast(sf::Vector2f start, float angle, float length) const;
	float getTargetDist(sf::Vector2f pos, size_t target) const;
	float getTargetDir(sf::Vector2f pos, size_t target) const;
	float getTargetRadius() const { return targetRadius; }
	size_t getTargetCount() const { return targets.size(); }

private:
	bool isVisualInit = false;
	std::vector<sf::Vector2f> targets;
	float targetRadius = 0.0f;
	std::vector<sf::CircleShape> targetShapes;
	std::vector<Body> worldBodies;
	std::vector<sf::RectangleShape> worldShapes;
};

#include "stdafx.h"
#include "VectorListTargetGenepool.h"

VectorListTargetGenepool::VectorListTargetGenepool(
	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
	sf::Vector2f targetPos, float targetRadius)
	: Genepool(createGenomeFn, createAgentFn), targetPos(targetPos), targetRadius(targetRadius)
{
	initVisual();
}

void VectorListTargetGenepool::initVisual()
{
	// Initialize variables
	target.setPosition(targetPos);
	target.setRadius(targetRadius);
	target.setOrigin({ targetRadius, targetRadius });
	target.setFillColor(sf::Color::Transparent);
	target.setOutlineColor(sf::Color::White);
	target.setOutlineThickness(1.0f);
};

void VectorListTargetGenepool::render(sf::RenderWindow* window)
{
	Genepool::render(window);

	// Draw target
	window->draw(target);
}

sf::Vector2f VectorListTargetGenepool::getTargetPos() const { return targetPos; }

float VectorListTargetGenepool::getTargetRadius() const { return targetRadius; }

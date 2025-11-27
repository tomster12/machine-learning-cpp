#include "stdafx.h"
#include "VectorListTargetGenepool.h"

VectorListTargetGenepool::VectorListTargetGenepool(
	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
	sf::Vector2f targetPos, float targetRadius)
	: Genepool(createGenomeFn, createAgentFn), targetPos(targetPos), targetRadius(targetRadius)
{}

void VectorListTargetGenepool::initVisual()
{
	if (isVisualInit) return;

	target.setPosition(targetPos);
	target.setRadius(targetRadius);
	target.setOrigin({ targetRadius, targetRadius });
	target.setFillColor(sf::Color::Transparent);
	target.setOutlineColor(sf::Color::White);
	target.setOutlineThickness(1.0f);

	isVisualInit = true;
};

void VectorListTargetGenepool::render(sf::RenderWindow* window)
{
	if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot render because uninitialized.");

	if (!isVisualInit) this->initVisual();

	for (const auto& inst : agentPopulation) inst->render(window);
	window->draw(target);
}

sf::Vector2f VectorListTargetGenepool::getTargetPos() const { return targetPos; }

float VectorListTargetGenepool::getTargetRadius() const { return targetRadius; }

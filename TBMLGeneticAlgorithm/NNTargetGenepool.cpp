#include "stdafx.h"

//#include "global.h"
//#include "NNTargetGenepool.h"
//#include "CommonImpl.h"
//#include "Utility.h"
//#include "Tensor.h"
//
//NNTargetGenepool::NNTargetGenepool(
//	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
//	std::vector<sf::Vector2f> targets, float targetRadius)
//	: Genepool(createGenomeFn, createAgentFn), targetPos(targets), targetRadius(targetRadius)
//{
//	this->initVisual();
//};
//
//void NNTargetGenepool::initVisual()
//{
//	// Initialize variables
//	targetShapes = std::vector<sf::CircleShape>();
//	for (auto& target : targetPos)
//	{
//		sf::CircleShape shape = sf::CircleShape();
//		shape.setRadius(targetRadius);
//		shape.setOrigin({ targetRadius, targetRadius });
//		shape.setFillColor(sf::Color::Transparent);
//		shape.setOutlineColor(sf::Color::White);
//		shape.setOutlineThickness(1.0f);
//		shape.setPosition(target);
//		targetShapes.push_back(shape);
//	}
//};
//
//void NNTargetGenepool::render(sf::RenderWindow* window)
//{
//	Genepool::render(window);
//	if (!this->showVisuals) return;
//	for (auto& shape : targetShapes) window->draw(shape);
//}
//
//const sf::Vector2f& NNTargetGenepool::getTarget(int index) const { return targetPos[index % targetPos.size()]; }
//
//size_t NNTargetGenepool::getTargetCount() const { return targetPos.size(); }
//
//float NNTargetGenepool::getTargetRadius() const { return targetRadius; }

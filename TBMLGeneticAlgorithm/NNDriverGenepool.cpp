#include "stdafx.h"
#include "NNDriverGenepool.h"

NNDriverGenepool::NNDriverGenepool(
	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
	std::vector<sf::Vector2f> targets, float targetRadius, std::vector<Body> worldBodies)
	: Genepool(createGenomeFn, createAgentFn),
	targets(targets), targetRadius(targetRadius), worldBodies(worldBodies)
{
	this->initVisual();
}

void NNDriverGenepool::initVisual()
{
	// Set up target shapes
	for (const auto& target : targets)
	{
		sf::CircleShape shape;
		shape.setPosition(target);
		shape.setFillColor(sf::Color::Transparent);
		shape.setOutlineColor(sf::Color::Green);
		shape.setOutlineThickness(1.0f);
		shape.setRadius(targetRadius);
		shape.setOrigin({ targetRadius, targetRadius });
		targetShapes.push_back(shape);
	}

	// Set up world shapes
	for (auto& body : worldBodies)
	{
		sf::RectangleShape shape;
		shape.setFillColor(sf::Color::Transparent);
		shape.setOutlineColor(sf::Color::White);
		shape.setOutlineThickness(1.0f);
		shape.setSize(body.size);
		shape.setOrigin({ body.size.x / 2.0f, body.size.y / 2.0f });
		body.updateShape(shape);
		body.recalculateVertices();
		worldShapes.push_back(shape);
	}
}

void NNDriverGenepool::render(sf::RenderWindow* window)
{
	Genepool::render(window);
	if (!this->showVisuals) return;

	// Draw target shapes
	for (const auto& shape : targetShapes) window->draw(shape);

	// Draw world shapes
	for (const auto& shape : worldShapes) window->draw(shape);
}

bool NNDriverGenepool::checkWorldIntersectBody(Body& body) const
{
	// Check if body intersects any other body
	for (const auto& other : worldBodies)
	{
		if (body.intersectBody(other)) return true;
	}
	return false;
}

bool NNDriverGenepool::checkWorldIntersectRaycast(sf::Vector2f start, float angle, float length) const
{
	// Get end point of raycast
	sf::Vector2f end = start + sf::Vector2f(std::cos(angle), std::sin(angle)) * length;

	// Check if ray intersects any body
	for (const auto& body : worldBodies)
	{
		if (body.intersectRaycast(start, end)) return true;
	}
	return false;
}

float NNDriverGenepool::getTargetDist(sf::Vector2f pos, size_t target) const
{
	float dx = targets[target].x - pos.x;
	float dy = targets[target].y - pos.y;
	return sqrt(dx * dx + dy * dy) - targetRadius;
}

float NNDriverGenepool::getTargetDir(sf::Vector2f pos, size_t target) const
{
	float dx = targets[target].x - pos.x;
	float dy = targets[target].y - pos.y;
	return atan2(dy, dx);
}

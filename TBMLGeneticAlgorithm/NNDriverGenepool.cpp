#include "stdafx.h"
#include "NNDriverGenepool.h"

NNDriverGenepool::NNDriverGenepool(
	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
	std::vector<sf::Vector2f> targets, float targetRadius, std::vector<Body> worldBodies)
	: Genepool(createGenomeFn, createAgentFn),
	targets(targets), targetRadius(targetRadius), worldBodies(worldBodies)
{}

void NNDriverGenepool::initVisual()
{
	if (isVisualInit) return;

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

	isVisualInit = true;
}

void NNDriverGenepool::render(sf::RenderWindow* window)
{
	if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot render because uninitialized.");

	if (!isVisualInit) this->initVisual();

	for (const auto& shape : targetShapes) window->draw(shape);
	for (const auto& shape : worldShapes) window->draw(shape);
	for (const auto& inst : agentPopulation) inst->render(window);
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

float NNDriverGenepool::getTargetDirDiff(const Body& body, size_t target) const
{
	float dx = targets[target].x - body.pos.x;
	float dy = targets[target].y - body.pos.y;

	float targetAngle = atan2(dy, dx);
	float diff = targetAngle - body.rot;

	return normalizeAngle(diff);
}

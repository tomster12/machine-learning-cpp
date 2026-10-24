#include "stdafx.h"
#include "NNDriverGenepool.h"
#include "UITheme.h"

NNDriverGenepool::NNDriverGenepool(
	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
	std::vector<sf::Vector2f> targets, float targetRadius, std::vector<Body> worldBodies)
	: Genepool(createGenomeFn, createAgentFn),
	targets(targets), targetRadius(targetRadius), worldBodies(worldBodies)
{
	for (auto& body : this->worldBodies)
	{
		body.recalculateVertices();
	}
}

void NNDriverGenepool::initVisual()
{
	if (isVisualInit) return;

	for (const auto& target : targets)
	{
		sf::CircleShape shape;
		shape.setPosition(target);
		shape.setOutlineThickness(1.5f);
		shape.setRadius(targetRadius);
		shape.setOrigin({ targetRadius, targetRadius });
		targetShapes.push_back(shape);
	}

	for (auto& body : worldBodies)
	{
		sf::RectangleShape shape;
		shape.setFillColor(sf::Color(139, 94, 60));
		shape.setOutlineColor(sf::Color(78, 50, 28));
		shape.setOutlineThickness(3.0f);
		shape.setSize(body.size);
		shape.setOrigin({ body.size.x / 2.0f, body.size.y / 2.0f });
		body.updateShape(shape);
		worldShapes.push_back(shape);
	}

	isVisualInit = true;
}

void NNDriverGenepool::renderWorld(sf::RenderWindow* window, const NNDriverAgent* elite)
{
	if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot render because uninitialized.");

	if (!isVisualInit) this->initVisual();

	// Targets: dim once the elite has passed them, bright for the one it is chasing
	const size_t eliteTarget = elite != nullptr ? elite->getCurrentTarget() : 0;
	for (size_t i = 0; i < targetShapes.size(); i++)
	{
		const bool passed = elite != nullptr && i < eliteTarget;
		const bool current = elite != nullptr && i == eliteTarget;
		targetShapes[i].setFillColor(sf::Color(255, 255, 255, current ? 56 : passed ? 10 : 24));
		targetShapes[i].setOutlineColor(sf::Color(255, 255, 255, current ? 230 : passed ? 50 : 110));
		window->draw(targetShapes[i]);

		theme::drawText(window, std::to_string(i + 1), targets[i], theme::TEXT_BODY, sf::Color(255, 255, 255, current ? 255 : passed ? 90 : 190), theme::Align::CENTER);
	}

	for (const auto& shape : worldShapes) window->draw(shape);
}

void NNDriverGenepool::renderAgents(sf::RenderWindow* window, NNDriverAgent* elite, AgentView view)
{
	if (view == AgentView::NONE) return;

	// Whole pack in one draw call, elite is drawn separately on top
	if (view == AgentView::ALL)
	{
		packVertices.clear();
		for (const auto& inst : agentPopulation)
		{
			if (inst.get() != elite) inst->appendPackVertices(packVertices);
		}
		window->draw(packVertices);
	}

	if (!elite->getFinished() && elite->getCurrentTarget() < targets.size())
	{
		const sf::Color lineColour(255, 255, 255, 90);
		const sf::Vertex line[2] = { { elite->getPosition(), lineColour }, { targets[elite->getCurrentTarget()], lineColour } };
		window->draw(line, 2, sf::PrimitiveType::Lines);
	}
	elite->renderElite(window);
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

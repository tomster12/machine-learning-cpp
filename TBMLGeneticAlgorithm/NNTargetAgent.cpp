#include "stdafx.h"
#include "NNTargetGenepool.h"
#include "NNTargetAgent.h"

NNTargetAgent::NNTargetAgent(
	NNTargetAgent::GenomeCPtr&& genome, const NNTargetGenepool* genepool,
	sf::Vector2f startPos, float radius, float moveAcc, float moveDrag, int maxIterations)
	: Agent(std::move(genome)), genepool(genepool), network(this->genome->copyNetwork()), netInput({ 1, 4 }, 0.0f),
	pos(startPos), radius(radius), moveAcc(moveAcc), moveDrag(moveDrag), maxIterations(maxIterations),
	currentIteration(0), currentTarget(0), vel(), anger(0.0f)
{}

void NNTargetAgent::initVisual()
{
	if (isVisualInit) return;

	// Set up shape
	shape.setRadius(radius);
	shape.setOrigin({ radius, radius });
	shape.setFillColor(sf::Color::Transparent);
	shape.setOutlineColor(sf::Color::White);
	shape.setOutlineThickness(1.0f);

	isVisualInit = true;
}

bool NNTargetAgent::evaluate()
{
	if (isFinished) return true;

	// Calculate with brain
	const sf::Vector2f& targetPos1 = genepool->getTarget(currentTarget);
	const sf::Vector2f& targetPos2 = genepool->getTarget(currentTarget + 1);
	netInput.setData({ targetPos1.x - pos.x, targetPos1.y - pos.y, vel.x, vel.y });
	const tbml::Tensor* output = network.propogatePtr(&netInput);

	// Update position, velocity, drag
	vel.x += output->at(0, 0) * moveAcc * (1.0f / 60.0f);
	vel.y += output->at(0, 1) * moveAcc * (1.0f / 60.0f);
	pos.x += vel.x * (1.0f / 60.0f);
	pos.y += vel.y * (1.0f / 60.0f);
	vel.x *= moveDrag;
	vel.y *= moveDrag;
	currentIteration++;

	// Check finish conditions
	float dist = calculateDist();
	anger += dist;
	if (dist <= 0.0f) currentTarget++;
	if (currentIteration == maxIterations)
	{
		isFinished = true;
		this->calculateFitness();
	}
	return isFinished;
};

void NNTargetAgent::render(sf::RenderWindow* window)
{
	if (!isVisualInit) initVisual();

	// Update shape to position
	shape.setPosition({ pos.x, pos.y });

	// Set color based on fitness
	this->calculateFitness();
	int v = static_cast<int>(255.0f * (0.3f + 0.7f * (fitness / 30.0f)));
	shape.setOutlineColor(sf::Color(v, v, v));

	window->draw(shape);
};

float NNTargetAgent::calculateDist()
{
	// Calculate distance to target
	sf::Vector2f targetPos = genepool->getTarget(currentTarget);
	float dx = targetPos.x - pos.x;
	float dy = targetPos.y - pos.y;
	float fullDistSq = sqrt(dx * dx + dy * dy);
	float radii = genepool->getTargetRadius();
	return fullDistSq - radii - radius;
}

float NNTargetAgent::calculateFitness()
{
	/*
	// Calculate fitness (anger)
	fitness = std::min(1000000.0f / anger, 15.0f);
	fitness -= currentTarget * 2.0f;
	fitness = fitness > 0.0f ? fitness : 0.0f;
	*/

	// Calculate fitness (speed)
	fitness = currentTarget + 1.0f - 1.0f / calculateDist();

	return fitness;
};

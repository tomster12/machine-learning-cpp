#include "stdafx.h"
#include "NNPoleBalancerAgent.h"
#include "Utility.h"

NNPoleBalancerAgent::NNPoleBalancerAgent(
	NNPoleBalancerAgent::GenomeCPtr&& genome,
	float cartMass, float poleMass, float poleLength, float force,
	float trackLimit, float angleLimit, float timeLimit)
	: Agent(std::move(genome)), network(this->genome->copyNetwork()), netInput({ 1, 4 }, 0.0f),
	cartMass(cartMass), poleMass(poleMass), poleLength(poleLength), force(force),
	trackLimit(trackLimit), angleLimit(angleLimit), timeLimit(timeLimit), poleAngle(0.1f)
{}

void NNPoleBalancerAgent::initVisual()
{
	if (isVisualInit) return;

	// Set up shapes
	cartShape.setSize({ 0.3f * METRE_TO_UNIT, 0.22f * METRE_TO_UNIT });
	cartShape.setOrigin({ 0.5f * (0.3f * METRE_TO_UNIT), 0.5f * (0.32f * METRE_TO_UNIT) });
	cartShape.setFillColor(sf::Color::Transparent);
	cartShape.setOutlineColor(sf::Color::White);
	cartShape.setOutlineThickness(1.0f);

	poleShape.setSize({ 5.0f, poleLength * METRE_TO_UNIT * 2 });
	poleShape.setOrigin({ 0.5f * 5.0f, poleLength * METRE_TO_UNIT * 2 });
	poleShape.setFillColor(sf::Color::Transparent);
	poleShape.setOutlineColor(sf::Color::White);
	poleShape.setOutlineThickness(1.0f);

	isVisualInit = true;
}

bool NNPoleBalancerAgent::evaluate()
{
	if (isFinished) return true;

	// Calculate force with network
	netInput.setData({ cartPosition, cartVelocity, poleAngle, poleVelocity });
	const tbml::Tensor* netOutput = network.propogatePtr(&netInput);
	float ft = netOutput->at(0, 0) * force;

	// Calculate acceleration
	cartAcceleration = (ft + poleMass * poleLength * (poleVelocity * poleVelocity * sin(poleAngle) - poleAcceleration * cos(poleAngle))) / (cartMass + poleMass);
	poleAcceleration = G * (sin(poleAngle) + cos(poleAngle) * (-ft - poleMass * poleLength * poleVelocity * poleVelocity * sin(poleAngle)) / (cartMass + poleMass)) / (poleLength * (4.0f / 3.0f - (poleMass * cos(poleAngle) * cos(poleAngle)) / (cartMass + poleMass)));

	// Update dynamics
	cartPosition = cartPosition + cartVelocity * TIME_STEP;
	poleAngle = poleAngle + poleVelocity * TIME_STEP;
	poleAngle = fmod(poleAngle + 3.141592653f, 2.0f * 3.141592653f) - 3.141592653f;
	cartVelocity += cartAcceleration * TIME_STEP;
	poleVelocity += poleAcceleration * TIME_STEP;
	this->time += TIME_STEP;

	// Add score if angle upright, less the further from the centre of the track
	if (abs(poleAngle) < angleLimit)
	{
		fitness += 1.0f - 0.5f * std::min(1.0f, abs(cartPosition) / trackLimit);
	}

	// Check finish conditions
	bool done = false;
	done |= abs(cartPosition) > trackLimit;
	done |= abs(poleAngle) > angleLimit;
	done |= time > timeLimit;
	if (done) isFinished = true;

	return isFinished;
}

void NNPoleBalancerAgent::render(sf::RenderWindow* window, bool isElite)
{
	if (!isVisualInit) initVisual();

	// Update shape positions and rotations
	cartShape.setPosition({ 700.0f + cartPosition * METRE_TO_UNIT, 700.0f });
	poleShape.setPosition({ 700.0f + cartPosition * METRE_TO_UNIT, 700.0f });
	poleShape.setRotation(sf::degrees(poleAngle));

	// Faint when part of the pack, solid for the elite
	sf::Color colour = sf::Color(255, 255, 255, 10);
	if (isElite) colour = sf::Color::White;
	else if (isFinished) colour = sf::Color(100, 100, 140, 5);
	else if (abs(poleAngle) > angleLimit) colour = sf::Color(170, 100, 100, 10);
	cartShape.setOutlineColor(colour);
	poleShape.setOutlineColor(colour);

	// Draw both to screen
	window->draw(this->cartShape);
	window->draw(this->poleShape);
}

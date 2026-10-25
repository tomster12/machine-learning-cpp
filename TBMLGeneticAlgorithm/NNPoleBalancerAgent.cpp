#include "stdafx.h"
#include "NNPoleBalancerAgent.h"
#include "UITheme.h"
#include "Utility.h"

NNPoleBalancerAgent::NNPoleBalancerAgent(
	NNPoleBalancerAgent::GenomeCPtr&& genome,
	float cartMass, float poleMass, float poleLength, float force,
	float trackLimit, float angleLimit, float timeLimit,
	float startAngle, float startPosition)
	: Agent(std::move(genome)), network(this->genome->copyNetwork()), netInput({ 1, 4 }, 0.0f),
	cartMass(cartMass), poleMass(poleMass), poleLength(poleLength), force(force),
	trackLimit(trackLimit), angleLimit(angleLimit), timeLimit(timeLimit),
	startAngle(startAngle), startPosition(startPosition), poleAngle(startAngle), cartPosition(startPosition)
{}

bool NNPoleBalancerAgent::evaluate()
{
	if (isFinished) return true;

	// Calculate force with network
	netInput.setData({ cartPosition, cartVelocity, poleAngle, poleVelocity });
	const tbml::Tensor* netOutput = network.propogatePtr(&netInput);
	push = netOutput->at(0, 0);
	float ft = push * force;

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
	step++;

	// Add score if angle upright, less the further from the centre of the track
	if (abs(poleAngle) < angleLimit)
	{
		fitness += 1.0f - 0.5f * std::min(1.0f, abs(cartPosition) / trackLimit);
	}

	// Check finish conditions
	if (abs(cartPosition) > trackLimit) outcome = PoleOutcome::OFF_TRACK;
	else if (abs(poleAngle) > angleLimit) outcome = PoleOutcome::FELL;
	else if (time > timeLimit) outcome = PoleOutcome::SURVIVED;
	if (outcome != PoleOutcome::BALANCING) isFinished = true;

	return isFinished;
}

void NNPoleBalancerAgent::appendPackVertices(sf::VertexArray& vertices) const
{
	// Faint ghost of the cart and pole, fainter still once finished
	const sf::Color colour = this->stateColour(isFinished ? 10 : 26);
	const sf::Vector2f centre = this->getCartCentre();
	const float halfWidth = CART_WIDTH / 2.0f;
	const float halfHeight = CART_HEIGHT / 2.0f;

	theme::appendQuad(vertices,
		{ centre.x - halfWidth, centre.y - halfHeight }, { centre.x + halfWidth, centre.y - halfHeight },
		{ centre.x + halfWidth, centre.y + halfHeight }, { centre.x - halfWidth, centre.y + halfHeight }, colour);
	theme::appendLine(vertices, this->getPivotPosition(), this->getTipPosition(), 3.0f, colour);
}

void NNPoleBalancerAgent::renderElite(sf::RenderWindow* window) const
{
	const sf::Vector2f centre = this->getCartCentre();
	const sf::Vector2f pivot = this->getPivotPosition();
	const sf::Vector2f tip = this->getTipPosition();
	const sf::Color state = this->stateColour(255);
	const sf::Color dark(30, 32, 38);
	const float halfWidth = CART_WIDTH / 2.0f;
	const float halfHeight = CART_HEIGHT / 2.0f;

	sf::VertexArray vertices(sf::PrimitiveType::Triangles);

	// Wheels
	for (int side = -1; side <= 1; side += 2)
	{
		const sf::Vector2f wheel(centre.x + static_cast<float>(side) * 18.0f, centre.y + halfHeight);
		theme::appendDisc(vertices, wheel, WHEEL_RADIUS, dark, 16);
		theme::appendDisc(vertices, wheel, WHEEL_RADIUS * 0.45f, sf::Color(120, 126, 138), 10);
	}

	// Body with an accent stripe along the top
	theme::appendQuad(vertices,
		{ centre.x - halfWidth, centre.y - halfHeight }, { centre.x + halfWidth, centre.y - halfHeight },
		{ centre.x + halfWidth, centre.y + halfHeight }, { centre.x - halfWidth, centre.y + halfHeight }, sf::Color(58, 64, 76));
	theme::appendQuad(vertices,
		{ centre.x - halfWidth, centre.y - halfHeight }, { centre.x + halfWidth, centre.y - halfHeight },
		{ centre.x + halfWidth, centre.y - halfHeight + 4.0f }, { centre.x - halfWidth, centre.y - halfHeight + 4.0f }, state);

	// Force the network is pushing with, drawn as a bar out of the centre of the cart
	const sf::Vector2f barEnd(centre.x + push * (halfWidth - 6.0f), centre.y + 4.0f);
	theme::appendLine(vertices, { centre.x, centre.y + 4.0f }, barEnd, 5.0f, theme::INFO);
	theme::appendDisc(vertices, barEnd, 4.0f, theme::INFO);

	// Pole with a hub at the pivot and a weight on the end
	theme::appendLine(vertices, pivot, tip, 6.0f, state);
	theme::appendDisc(vertices, pivot, 6.0f, dark, 12);
	theme::appendDisc(vertices, tip, 11.0f, state, 16);
	theme::appendDisc(vertices, tip, 4.0f, dark, 10);
	window->draw(vertices);

	// Fitness readout on a small plate above the pole, kept upright
	const std::string label = global::formatFloat(fitness, 0);
	const float plateWidth = theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
	const sf::Vector2f plate(tip.x, tip.y - 30.0f);
	theme::drawRect(window, { plate.x - plateWidth / 2.0f, plate.y - 9.0f }, { plateWidth, 18.0f }, theme::PANEL_FILL);
	theme::drawText(window, label, plate, theme::TEXT_SMALL, theme::TEXT_PRIMARY, theme::Align::CENTER);
}

sf::Color NNPoleBalancerAgent::stateColour(std::uint8_t alpha) const
{
	sf::Color base = theme::CART_BALANCING;
	if (outcome == PoleOutcome::FELL) base = theme::CART_FELL;
	else if (outcome == PoleOutcome::OFF_TRACK) base = theme::CART_OFF_TRACK;
	else if (outcome == PoleOutcome::SURVIVED) base = theme::CART_SURVIVED;
	return sf::Color(base.r, base.g, base.b, alpha);
}

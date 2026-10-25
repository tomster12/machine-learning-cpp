#include "stdafx.h"
#include "NNTargetAgent.h"
#include "UITheme.h"

namespace
{
	constexpr float TIME_STEP = 1.0f / 60.0f;
}

NNTargetAgent::NNTargetAgent(
	NNTargetAgent::GenomeCPtr&& genome, std::shared_ptr<const Targets> targets, float targetRadius,
	sf::Vector2f startPos, float radius, float moveAcc, float moveDrag, int maxIterations)
	: Agent(std::move(genome)), targets(std::move(targets)), network(this->genome->copyNetwork()), netInput({ 1, 4 }, 0.0f),
	targetRadius(targetRadius), startPos(startPos), radius(radius), moveAcc(moveAcc), moveDrag(moveDrag), maxIterations(static_cast<size_t>(maxIterations)),
	pos(startPos)
{}

bool NNTargetAgent::evaluate()
{
	if (isFinished) return true;

	// Calculate with brain
	const sf::Vector2f toTarget = this->getTarget(currentTarget) - pos;
	netInput.setData({ toTarget.x, toTarget.y, vel.x, vel.y });
	const tbml::Tensor* output = network.propogatePtr(&netInput);

	// Update position, velocity, drag
	acc = { output->at(0, 0), output->at(0, 1) };
	vel += acc * moveAcc * TIME_STEP;
	pos += vel * TIME_STEP;
	vel *= moveDrag;
	currentIteration++;

	// Move on to the next target once touching this one
	if (this->calculateDist() <= 0.0f) currentTarget++;
	if (currentIteration == maxIterations) isFinished = true;

	// Keep fitness current so the elite can be picked mid generation
	this->calculateFitness();
	return isFinished;
}

void NNTargetAgent::appendPackVertices(sf::VertexArray& vertices) const
{
	const sf::Color base = theme::CAR_ALIVE;
	theme::appendDisc(vertices, pos, radius, sf::Color(base.r, base.g, base.b, isFinished ? 12 : 40), 8);
}

void NNTargetAgent::renderElite(sf::RenderWindow* window) const
{
	// Soft halo around a solid core
	sf::VertexArray vertices(sf::PrimitiveType::Triangles);
	theme::appendDisc(vertices, pos, radius * 3.0f, sf::Color(theme::ELITE.r, theme::ELITE.g, theme::ELITE.b, 24), 20);
	theme::appendDisc(vertices, pos, radius * 1.8f, sf::Color(theme::ELITE.r, theme::ELITE.g, theme::ELITE.b, 50), 20);
	theme::appendDisc(vertices, pos, radius, sf::Color(244, 246, 248), 16);
	window->draw(vertices);

	// Fitness readout on a small plate above the agent, kept upright
	const std::string label = global::formatFloat(fitness, 1);
	const float plateWidth = theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
	const sf::Vector2f centre(pos.x, pos.y - radius * 3.0f - 12.0f);
	theme::drawRect(window, { centre.x - plateWidth / 2.0f, centre.y - 9.0f }, { plateWidth, 18.0f }, theme::PANEL_FILL);
	theme::drawText(window, label, centre, theme::TEXT_SMALL, theme::TEXT_PRIMARY, theme::Align::CENTER);
}

float NNTargetAgent::calculateDist() const
{
	// Gap between the edge of the agent and the edge of the target
	return (this->getTarget(currentTarget) - pos).length() - targetRadius - radius;
}

void NNTargetAgent::calculateFitness()
{
	// Whole targets reached, plus how far along the leg to the current one
	const sf::Vector2f from = currentTarget == 0 ? startPos : this->getTarget(currentTarget - 1);
	const sf::Vector2f to = this->getTarget(currentTarget);
	const float legLength = std::max((to - from).length(), 1.0f);
	const float progress = std::clamp(1.0f - (to - pos).length() / legLength, 0.0f, 1.0f);
	fitness = 1.0f + static_cast<float>(currentTarget) + progress;
}

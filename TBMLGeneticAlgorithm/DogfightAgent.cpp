#include "stdafx.h"
#include "DogfightAgent.h"
#include "Utility.h"
#include "UITheme.h"

DogfightAgent::DogfightAgent(DogfightAgent::GenomeCPtr&& genome, std::shared_ptr<const DogfightLevel> level, DogfightAgent::GenomeCPtr rival, int rivalGeneration)
	: Agent(std::move(genome)), pilot(this->genome->copyNetwork()), arena(std::move(level)), rivalGeneration(rivalGeneration)
{
	if (rival != nullptr) rivalPilot.emplace(rival->copyNetwork());
	this->calculateFitness();
}

bool DogfightAgent::evaluate()
{
	if (isFinished) return true;

	// Both pilots think every few steps and hold their controls in between, with no rival the other ship just drifts
	if (step % DECISION_INTERVAL == 0)
	{
		controls[0] = pilot.think(arena, 0);
		if (rivalPilot.has_value()) controls[1] = rivalPilot->think(arena, 1);
	}

	arena.step(controls[0], controls[1]);
	step++;
	if (arena.getShip(0).isAlive()) aliveSteps++;

	// Mutual destruction counts as a loss
	if (!arena.getShip(0).isAlive()) outcome = DogfightOutcome::LOST;
	else if (!arena.getShip(1).isAlive()) outcome = DogfightOutcome::WON;
	else if (step >= MAX_STEPS) outcome = DogfightOutcome::SURVIVED;
	isFinished = outcome != DogfightOutcome::FIGHTING;

	// Keep fitness current so the elite can be picked mid generation
	this->calculateFitness();
	return isFinished;
}

void DogfightAgent::calculateFitness()
{
	const DogfightArena::Tally& tally = arena.getTally(0);
	const float steps = static_cast<float>(MAX_STEPS);

	// Rocks shot, damage landed and time spent alive earn points, damage taken and wasted bullets lose them
	const float rocks = std::min(2.5f, tally.rockPoints / 1000.0f);
	const float damage = 1.5f * std::min(1.0f, tally.damageDealt / DogfightShip::MAX_HEALTH);
	const float survival = 0.3f * static_cast<float>(aliveSteps) / steps;
	const float hurt = 0.4f * std::min(1.0f, tally.damageTaken / DogfightShip::MAX_HEALTH);
	const float waste = std::min(0.25f, 0.002f * static_cast<float>(tally.missed));

	float score = rocks + damage + survival - hurt - waste;
	if (outcome == DogfightOutcome::WON) score += 1.0f + 0.5f * (1.0f - static_cast<float>(step) / steps);

	// Square fitness to sharpen selection
	score = std::max(0.0f, score);
	fitness = score * score;
}

sf::Color DogfightAgent::stateColour(std::uint8_t alpha) const
{
	sf::Color base = theme::DOGFIGHT_FIGHTING;
	if (outcome == DogfightOutcome::WON) base = theme::DOGFIGHT_WON;
	else if (outcome == DogfightOutcome::LOST) base = theme::DOGFIGHT_LOST;
	else if (outcome == DogfightOutcome::SURVIVED) base = theme::DOGFIGHT_SURVIVED;
	return sf::Color(base.r, base.g, base.b, alpha);
}

void DogfightAgent::appendPackVertices(sf::VertexArray& vertices) const
{
	// Colour shows the outcome, opacity shows role: ships still fighting are clearer than finished ones
	if (!arena.getShip(0).isAlive()) return;

	arena.getShip(0).appendGhost(vertices, this->stateColour(isFinished ? 60 : 110));
}

void DogfightAgent::renderElite(sf::RenderWindow* window) const
{
	const float time = static_cast<float>(step) * DogfightArena::TIME_STEP;
	arena.render(window, this->stateColour(255), theme::DOGFIGHT_RIVAL, time);

	// Thin ring around the elite so it can be found in a world this big
	const DogfightShip& ship = arena.getShip(0);
	if (!ship.isAlive()) return;

	const sf::Color colour = this->stateColour(140);
	for (const sf::Vector2f& offset : DogfightLevel::getImageOffsets(ship.getPosition(), 60.0f)) theme::drawRing(window, ship.getPosition() + offset, 45.0f, 2.5f, colour);
}

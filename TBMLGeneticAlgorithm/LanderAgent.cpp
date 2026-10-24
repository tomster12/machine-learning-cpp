#include "stdafx.h"
#include "LanderAgent.h"
#include "Utility.h"
#include "UITheme.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float TIME_STEP = 1.0f / 60.0f;
	constexpr float GRAVITY = 120.0f;
	constexpr float MAIN_THRUST = 330.0f;
	constexpr float TURN_ACC = 4.0f;
	constexpr float TURN_DAMPING = 0.995f;
	constexpr float FUEL_SECONDS = 8.0f;
	constexpr float TURN_FUEL_COST = 0.15f;
	constexpr size_t MAX_STEPS = 1500;
	constexpr float RAY_LENGTH = 300.0f;
	constexpr float FEET_OFFSET = 15.0f;
	constexpr float MAX_DOWN_SPEED = 60.0f;
	constexpr float MAX_SIDE_SPEED = 45.0f;
	constexpr float MAX_TILT = 0.3f;
	constexpr float MAX_SPIN = 1.5f;

	float wrapAngle(float angle) { return std::remainder(angle, 2.0f * PI); }
}

LanderAgent::LanderAgent(LanderAgent::GenomeCPtr&& genome, std::shared_ptr<const LanderCourse> course)
	: Agent(std::move(genome)), course(std::move(course)), network(this->genome->copyNetwork()), netInput({ 1, INPUT_COUNT }, 0.0f),
	pos(this->course->getSpawnPos()), vel(this->course->getSpawnVel()), rot(this->course->getSpawnRot())
{
	startDistance = std::max(1.0f, (this->course->getPadCentre() - pos).length());
	this->sense();
	this->calculateFitness();
}

bool LanderAgent::evaluate()
{
	if (isFinished) return true;

	// Think, with the engine and thrusters dead once the fuel is gone
	this->sense();
	const tbml::Tensor* output = network.propogatePtr(&netInput);
	throttle = fuel > 0.0f ? 0.5f * (output->at(0, 0) + 1.0f) : 0.0f;
	steer = fuel > 0.0f ? output->at(0, 1) : 0.0f;

	// Integrate motion
	const sf::Vector2f up(std::sin(rot), -std::cos(rot));
	vel += (up * (throttle * MAIN_THRUST) + sf::Vector2f(0.0f, GRAVITY)) * TIME_STEP;
	pos += vel * TIME_STEP;
	angVel = (angVel + steer * TURN_ACC * TIME_STEP) * TURN_DAMPING;
	rot += angVel * TIME_STEP;
	fuel = std::max(0.0f, fuel - (throttle + TURN_FUEL_COST * std::abs(steer)) * TIME_STEP / FUEL_SECONDS);
	step++;

	// Touchdown when the nose or either foot meets the ground
	const sf::Vector2f contacts[3] = { toWorld({ 0.0f, -17.0f }), toWorld({ -FEET_OFFSET, 17.0f }), toWorld({ FEET_OFFSET, 17.0f }) };
	bool touching = false;
	for (const sf::Vector2f& contact : contacts) touching |= contact.y >= course->getHeight(contact.x);

	if (touching)
	{
		const bool onPad = std::abs(pos.x - course->getPadCentre().x) <= course->getPadWidth() / 2.0f - FEET_OFFSET;
		const bool gentle = vel.y <= MAX_DOWN_SPEED && std::abs(vel.x) <= MAX_SIDE_SPEED && std::abs(wrapAngle(rot)) <= MAX_TILT && std::abs(angVel) <= MAX_SPIN;
		outcome = onPad && gentle ? LanderOutcome::LANDED : LanderOutcome::CRASHED;
		isFinished = true;
	}
	else if (pos.x < -100.0f || pos.x > LanderCourse::WIDTH + 100.0f || pos.y < -500.0f || step >= MAX_STEPS)
	{
		outcome = LanderOutcome::LOST;
		isFinished = true;
	}

	// Keep fitness current so the elite can be picked mid generation
	this->calculateFitness();
	return isFinished;
}

void LanderAgent::sense()
{
	// Everything is scaled to roughly -1 to 1 so no input drowns out the others
	const sf::Vector2f toPad = course->getPadCentre() - pos;
	netInput.at(0, 0) = toPad.x / 700.0f;
	netInput.at(0, 1) = toPad.y / 700.0f;
	netInput.at(0, 2) = vel.x / 300.0f;
	netInput.at(0, 3) = vel.y / 300.0f;
	netInput.at(0, 4) = wrapAngle(rot) / PI;
	netInput.at(0, 5) = angVel / 3.0f;
	netInput.at(0, 6) = fuel;

	// Fan of rays below the ship, closer ground reads higher
	for (size_t i = 0; i < RAY_COUNT; i++)
	{
		const float offset = (static_cast<float>(i) - 3.0f) * (PI / 6.0f);
		netInput.at(0, 7 + i) = 1.0f - course->castRay(pos, PI / 2.0f + rot + offset, RAY_LENGTH);
	}
}

void LanderAgent::calculateFitness()
{
	// Progress towards the pad since spawn, so a lucky spawn doesn't count as skill
	const float distance = (course->getPadCentre() - pos).length();
	const float approach = std::clamp(1.0f - distance / startDistance, 0.0f, 1.0f);
	const float straight = std::max(0.0f, 1.0f - std::abs(wrapAngle(rot)) / 0.6f);
	const float excess = std::max(0.0f, vel.y - MAX_DOWN_SPEED) + std::max(0.0f, std::abs(vel.x) - MAX_SIDE_SPEED);
	const float softness = 1.0f / (1.0f + excess / 60.0f);

	// Time spent slow, upright and closing on the pad is rewarded every step, so a controlled descent beats a dive
	const float controlled = approach * (0.4f + 0.6f / (1.0f + vel.length() / 80.0f)) * (0.5f + 0.5f * straight);
	if (outcome == LanderOutcome::FLYING) flightCredit += controlled;
	const float flightScore = std::min(1.0f, flightCredit / 400.0f);

	float score = 0.0f;
	switch (outcome)
	{
	case LanderOutcome::FLYING:
	case LanderOutcome::LOST:
		score = flightScore;
		break;

	case LanderOutcome::CRASHED:
		score = flightScore + 0.8f * softness * straight * (0.3f + 0.7f * approach);
		break;

	case LanderOutcome::LANDED:
		score = 3.0f + fuel + 0.5f * (1.0f - static_cast<float>(step) / static_cast<float>(MAX_STEPS)) + 0.5f * straight;
		break;
	}

	// Square fitness to sharpen selection
	fitness = score * score;
}

sf::Vector2f LanderAgent::toWorld(sf::Vector2f local) const
{
	const float c = std::cos(rot);
	const float s = std::sin(rot);
	return { pos.x + local.x * c - local.y * s, pos.y + local.x * s + local.y * c };
}

sf::Color LanderAgent::stateColour(std::uint8_t alpha) const
{
	sf::Color base = theme::SHIP_FLYING;
	if (outcome == LanderOutcome::LANDED) base = theme::SHIP_LANDED;
	else if (outcome == LanderOutcome::CRASHED) base = theme::SHIP_CRASHED;
	else if (outcome == LanderOutcome::LOST) base = theme::SHIP_LOST;
	return sf::Color(base.r, base.g, base.b, alpha);
}

void LanderAgent::appendHull(sf::VertexArray& vertices, sf::Color colour, sf::Color shade) const
{
	// Capsule body plus a darker base plate
	vertices.append({ toWorld({ 0.0f, -17.0f }), colour });
	vertices.append({ toWorld({ -9.0f, 7.0f }), colour });
	vertices.append({ toWorld({ 9.0f, 7.0f }), colour });

	vertices.append({ toWorld({ -12.0f, 7.0f }), shade });
	vertices.append({ toWorld({ 12.0f, 7.0f }), shade });
	vertices.append({ toWorld({ 12.0f, 12.0f }), shade });
	vertices.append({ toWorld({ -12.0f, 7.0f }), shade });
	vertices.append({ toWorld({ 12.0f, 12.0f }), shade });
	vertices.append({ toWorld({ -12.0f, 12.0f }), shade });
}

void LanderAgent::appendPackVertices(sf::VertexArray& vertices) const
{
	// Colour shows the outcome, opacity shows role: ships still flying are clearer than finished ones
	const sf::Color colour = stateColour(isFinished ? 70 : 130);
	this->appendHull(vertices, colour, sf::Color(colour.r / 2, colour.g / 2, colour.b / 2, colour.a));
}

void LanderAgent::renderElite(sf::RenderWindow* window) const
{
	const sf::Color colour = stateColour(255);

	// Soft halo so the elite reads against the pack
	sf::CircleShape halo(28.0f);
	halo.setOrigin({ 28.0f, 28.0f });
	halo.setPosition(pos);
	halo.setFillColor(sf::Color(colour.r, colour.g, colour.b, 22));
	window->draw(halo);

	// Main flame, drawn first so the hull sits on top
	if (!isFinished && throttle > 0.02f)
	{
		const float length = (10.0f + 34.0f * throttle) * (0.8f + 0.4f * tbml::fn::getRandomFloat());
		sf::VertexArray flame(sf::PrimitiveType::Triangles);
		flame.append({ toWorld({ -6.0f, 12.0f }), sf::Color(255, 140, 50, 210) });
		flame.append({ toWorld({ 6.0f, 12.0f }), sf::Color(255, 140, 50, 210) });
		flame.append({ toWorld({ 0.0f, 12.0f + length }), sf::Color(255, 90, 30, 0) });
		flame.append({ toWorld({ -3.0f, 12.0f }), sf::Color(255, 235, 160, 255) });
		flame.append({ toWorld({ 3.0f, 12.0f }), sf::Color(255, 235, 160, 255) });
		flame.append({ toWorld({ 0.0f, 12.0f + length * 0.55f }), sf::Color(255, 200, 100, 0) });
		window->draw(flame);
	}

	// Side thruster puff on whichever side is firing
	if (!isFinished && std::abs(steer) > 0.15f)
	{
		const float side = steer > 0.0f ? -1.0f : 1.0f;
		const float length = 6.0f + 10.0f * std::abs(steer);
		sf::VertexArray puff(sf::PrimitiveType::Triangles);
		puff.append({ toWorld({ side * 8.0f, -5.0f }), sf::Color(255, 255, 255, 220) });
		puff.append({ toWorld({ side * 8.0f, -1.0f }), sf::Color(255, 255, 255, 220) });
		puff.append({ toWorld({ side * (8.0f + length), -3.0f }), sf::Color(255, 255, 255, 0) });
		window->draw(puff);
	}

	sf::VertexArray hull(sf::PrimitiveType::Triangles);
	this->appendHull(hull, colour, sf::Color(colour.r * 6 / 10, colour.g * 6 / 10, colour.b * 6 / 10));
	window->draw(hull);

	// Landing legs
	const sf::Vertex legs[4] = {
		{ toWorld({ -9.0f, 9.0f }), colour }, { toWorld({ -FEET_OFFSET, 17.0f }), colour },
		{ toWorld({ 9.0f, 9.0f }), colour }, { toWorld({ FEET_OFFSET, 17.0f }), colour } };
	window->draw(legs, 4, sf::PrimitiveType::Lines);

	// Fitness readout on a small plate above the ship, kept upright
	const std::string label = global::formatFloat(fitness, 1);
	const float plateWidth = theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
	const sf::Vector2f centre(pos.x, pos.y - 40.0f);
	theme::drawRect(window, { centre.x - plateWidth / 2.0f, centre.y - 9.0f }, { plateWidth, 18.0f }, theme::PANEL_FILL);
	theme::drawText(window, label, centre, theme::TEXT_SMALL, theme::TEXT_PRIMARY, theme::Align::CENTER);
}

#include "stdafx.h"
#include <limits>
#include "DogfightPilot.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float RAY_LENGTH = 500.0f;
	constexpr float ROCK_RANGE = 600.0f;
	constexpr float ROCK_SPEED_RANGE = 200.0f;
	constexpr float ROCK_WARNING = 3.0f;
	constexpr float BULLET_WARNING = 1.5f;
	constexpr float FOE_RANGE = 1800.0f;

	float wrapAngle(float angle) { return std::remainder(angle, 2.0f * PI); }
	float dot(sf::Vector2f a, sf::Vector2f b) { return a.x * b.x + a.y * b.y; }
	float clampUnit(float value) { return std::clamp(value, -1.0f, 1.0f); }

	// Time until two things moving past each other are closest, or a negative number if they never come within reach
	float timeToApproach(sf::Vector2f relativePos, sf::Vector2f relativeVel, float reach, float maxTime)
	{
		const float speedSq = dot(relativeVel, relativeVel);
		if (speedSq < 0.001f) return dot(relativePos, relativePos) < reach * reach ? 0.0f : -1.0f;

		const float time = std::max(0.0f, -dot(relativePos, relativeVel) / speedSq);
		if (time > maxTime) return -1.0f;

		const sf::Vector2f closest = relativePos + relativeVel * time;
		return dot(closest, closest) < reach * reach ? time : -1.0f;
	}
}

DogfightPilot::DogfightPilot(tbml::nn::NeuralNetwork&& network)
	: network(std::move(network)), netInput({ 1, INPUT_COUNT }, 0.0f)
{}

DogfightShip::Controls DogfightPilot::think(const DogfightArena& arena, size_t shipIndex)
{
	this->sense(arena, shipIndex);
	const tbml::Tensor* output = network.propogatePtr(&netInput);

	// Thrust and turn are analogue, everything else is a switch
	DogfightShip::Controls controls;
	controls.thrust = 0.5f * (output->at(0, 0) + 1.0f);
	controls.turn = output->at(0, 1);
	controls.brake = output->at(0, 2) > 0.5f;
	controls.shoot = output->at(0, 3) > 0.0f;
	controls.boost = output->at(0, 4) > 0.5f;
	controls.shield = output->at(0, 5) > 0.5f;
	return controls;
}

void DogfightPilot::sense(const DogfightArena& arena, size_t shipIndex)
{
	const DogfightShip& self = arena.getShip(shipIndex);
	const DogfightShip& foe = arena.getShip(1 - shipIndex);
	const DogfightLevel& level = arena.getLevel();
	const sf::Vector2f heading = self.getHeading();
	const sf::Vector2f side(-heading.y, heading.x);
	auto local = [&](sf::Vector2f v) { return sf::Vector2f(dot(v, heading), dot(v, side)); };

	size_t cursor = 0;
	auto put = [&](float value) { netInput.at(0, cursor++) = value; };

	// Own motion and which tools are ready, abilities the level hasn't unlocked read as never ready
	const sf::Vector2f velocity = local(self.getVelocity());
	put(velocity.x / DogfightShip::MAX_SPEED);
	put(velocity.y / DogfightShip::MAX_SPEED);
	put(self.getHealthFraction());
	put(self.getGunReadiness());
	put(level.isBoostEnabled() ? self.getBoostReadiness() : 0.0f);
	put(level.isShieldEnabled() ? self.getShieldReadiness() : 0.0f);
	put(self.getShieldRemaining());

	// Fan of rays around the ship, closer rock reads higher, the first one points straight down the gun
	const std::vector<DogfightArena::Asteroid>& asteroids = arena.getAsteroids();
	for (size_t i = 0; i < RAY_COUNT; i++)
	{
		const float angle = self.getRotation() + static_cast<float>(i) * (2.0f * PI / static_cast<float>(RAY_COUNT));
		const sf::Vector2f direction(std::cos(angle), std::sin(angle));
		float nearest = RAY_LENGTH;
		for (const DogfightArena::Asteroid& asteroid : asteroids)
		{
			const sf::Vector2f delta = DogfightLevel::wrapDelta(self.getPosition(), asteroid.pos);
			const float along = dot(delta, direction);
			if (along <= 0.0f) continue;

			const float reach = asteroid.getRadius() + 6.0f;
			const float offAxisSq = dot(delta, delta) - along * along;
			if (offAxisSq >= reach * reach) continue;

			nearest = std::min(nearest, std::max(0.0f, along - std::sqrt(reach * reach - offAxisSq)));
		}
		put(1.0f - nearest / RAY_LENGTH);
	}

	// The nearest rocks by their surface, with where they are and how they are moving, in the ship's own frame
	struct RockCandidate { float distance = std::numeric_limits<float>::max(); const DogfightArena::Asteroid* asteroid = nullptr; };
	RockCandidate nearestRocks[NEAREST_ROCKS];
	for (const DogfightArena::Asteroid& asteroid : asteroids)
	{
		RockCandidate candidate{ DogfightLevel::wrapDelta(self.getPosition(), asteroid.pos).length() - asteroid.getRadius(), &asteroid };
		for (RockCandidate& slot : nearestRocks)
		{
			if (candidate.distance < slot.distance) std::swap(candidate, slot);
		}
	}
	for (const RockCandidate& candidate : nearestRocks)
	{
		if (candidate.asteroid == nullptr)
		{
			for (int i = 0; i < 4; i++) put(0.0f);
			continue;
		}

		const sf::Vector2f position = local(DogfightLevel::wrapDelta(self.getPosition(), candidate.asteroid->pos));
		const sf::Vector2f relativeVel = local(candidate.asteroid->vel - self.getVelocity());
		put(clampUnit(position.x / ROCK_RANGE));
		put(clampUnit(position.y / ROCK_RANGE));
		put(clampUnit(relativeVel.x / ROCK_SPEED_RANGE));
		put(clampUnit(relativeVel.y / ROCK_SPEED_RANGE));
	}

	// Most imminent collision with any rock and which way it lies
	float soonest = -1.0f;
	sf::Vector2f threatPos;
	for (const DogfightArena::Asteroid& asteroid : asteroids)
	{
		const sf::Vector2f delta = DogfightLevel::wrapDelta(self.getPosition(), asteroid.pos);
		const float time = timeToApproach(delta, asteroid.vel - self.getVelocity(), asteroid.getRadius() + DogfightShip::RADIUS + 10.0f, ROCK_WARNING);
		if (time >= 0.0f && (soonest < 0.0f || time < soonest))
		{
			soonest = time;
			threatPos = delta;
		}
	}
	if (soonest < 0.0f)
	{
		put(0.0f);
		put(0.0f);
	}
	else
	{
		const sf::Vector2f threat = local(threatPos);
		put(1.0f - soonest / ROCK_WARNING);
		put(std::atan2(threat.y, threat.x) / PI);
	}

	// The rival: where it is, whether it is aiming at me, where to aim to hit it and how hurt it is
	const sf::Vector2f toFoe = DogfightLevel::wrapDelta(self.getPosition(), foe.getPosition());
	const float distance = toFoe.length();
	const sf::Vector2f foeLocal = local(toFoe);
	const float foeAim = wrapAngle(std::atan2(-toFoe.y, -toFoe.x) - foe.getRotation());
	const sf::Vector2f lead = local(toFoe + (foe.getVelocity() - self.getVelocity()) * (distance / DogfightShip::BULLET_SPEED));
	put(std::atan2(foeLocal.y, foeLocal.x) / PI);
	put(clampUnit(distance / FOE_RANGE));
	put(foeAim / PI);
	put(std::atan2(lead.y, lead.x) / PI);
	put(foe.getHealthFraction());

	// Soonest hostile bullet that would hit and which way it is coming from
	float soonestShot = -1.0f;
	sf::Vector2f shotPos;
	for (const DogfightArena::Bullet& bullet : arena.getBullets())
	{
		if (bullet.owner == shipIndex) continue;

		const sf::Vector2f delta = DogfightLevel::wrapDelta(self.getPosition(), bullet.pos);
		const float time = timeToApproach(delta, bullet.vel - self.getVelocity(), DogfightShip::RADIUS + DogfightShip::BULLET_RADIUS + 4.0f, BULLET_WARNING);
		if (time >= 0.0f && (soonestShot < 0.0f || time < soonestShot))
		{
			soonestShot = time;
			shotPos = delta;
		}
	}
	if (soonestShot < 0.0f)
	{
		put(0.0f);
		put(0.0f);
	}
	else
	{
		const sf::Vector2f shot = local(shotPos);
		put(1.0f - soonestShot / BULLET_WARNING);
		put(std::atan2(shot.y, shot.x) / PI);
	}
}

#include "stdafx.h"
#include "DogfightShip.h"
#include "DogfightLevel.h"
#include "UITheme.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float TURN_EASING = 14.0f;
	constexpr float OVERSPEED_BRAKING = 900.0f;
	constexpr float SCALE = DogfightShip::RADIUS / 14.0f;
	constexpr float HEALTH_BAR_WIDTH = DogfightShip::RADIUS * 3.5f;

	sf::Color shade(sf::Color colour, float factor, std::uint8_t alpha)
	{
		return sf::Color(static_cast<std::uint8_t>(colour.r * factor), static_cast<std::uint8_t>(colour.g * factor), static_cast<std::uint8_t>(colour.b * factor), alpha);
	}
}

DogfightShip::DogfightShip(sf::Vector2f pos, float rot)
	: pos(pos), rot(rot)
{}

void DogfightShip::control(const Controls& newControls, bool boostEnabled, bool shieldEnabled)
{
	controls = newControls;
	controls.thrust = std::clamp(controls.thrust, 0.0f, 1.0f);
	controls.turn = std::clamp(controls.turn, -1.0f, 1.0f);

	// Abilities fire once their cooldown has run out, and only if the level has unlocked them
	if (controls.boost && boostEnabled && boostCooldown <= 0.0f)
	{
		boostTimer = BOOST_TIME;
		boostCooldown = BOOST_COOLDOWN;
		boostStarted = true;
	}
	if (controls.shield && shieldEnabled && shieldCooldown <= 0.0f)
	{
		shieldTimer = SHIELD_TIME;
		shieldCooldown = SHIELD_COOLDOWN;
		shieldStarted = true;
	}
}

void DogfightShip::step(float dt)
{
	// Turning eases towards the requested rate so the nose doesn't snap
	angVel += (controls.turn * TURN_SPEED - angVel) * std::min(1.0f, dt * TURN_EASING);
	rot = std::remainder(rot + angVel * dt, 2.0f * PI);

	float acceleration = controls.thrust * THRUST;
	if (boostTimer > 0.0f) acceleration += BOOST_THRUST;
	vel += this->getHeading() * (acceleration * dt);
	vel *= std::exp(-(controls.brake ? BRAKE_DRAG : DRAG) * dt);

	// Anything above the speed limit bleeds off quickly, which is what ends a boost
	const float limit = boostTimer > 0.0f ? BOOST_SPEED : MAX_SPEED;
	const float speed = vel.length();
	if (speed > limit) vel *= (speed - std::min(speed - limit, OVERSPEED_BRAKING * dt)) / speed;

	pos = DogfightLevel::wrapPosition(pos + vel * dt);

	gunTimer = std::max(0.0f, gunTimer - dt);
	boostTimer = std::max(0.0f, boostTimer - dt);
	boostCooldown = std::max(0.0f, boostCooldown - dt);
	shieldTimer = std::max(0.0f, shieldTimer - dt);
	shieldCooldown = std::max(0.0f, shieldCooldown - dt);
	graceTimer = std::max(0.0f, graceTimer - dt);
}

bool DogfightShip::takeShot()
{
	if (!controls.shoot || gunTimer > 0.0f || !this->isAlive()) return false;

	gunTimer = GUN_COOLDOWN;
	return true;
}

float DogfightShip::takeDamage(float amount)
{
	// Shields and the brief grace after a hit soak everything
	if (shieldTimer > 0.0f || graceTimer > 0.0f || !this->isAlive()) return 0.0f;

	const float applied = std::min(amount, health);
	health -= amount;
	graceTimer = HIT_GRACE;
	return applied;
}

void DogfightShip::nudge(sf::Vector2f offset, sf::Vector2f velocityChange)
{
	pos = DogfightLevel::wrapPosition(pos + offset);
	vel += velocityChange;
}

void DogfightShip::appendGhost(sf::VertexArray& vertices, sf::Color colour) const
{
	const sf::Vector2f forward = this->getHeading();
	const sf::Vector2f side(-forward.y, forward.x);
	vertices.append({ pos + forward * (13.0f * SCALE), colour });
	vertices.append({ pos - forward * (9.0f * SCALE) - side * (8.0f * SCALE), colour });
	vertices.append({ pos - forward * (9.0f * SCALE) + side * (8.0f * SCALE), colour });
}

void DogfightShip::render(sf::RenderWindow* window, sf::Color colour, float time) const
{
	if (!this->isAlive()) return;

	for (const sf::Vector2f& offset : DogfightLevel::getImageOffsets(pos, 40.0f)) this->renderAt(window, pos + offset, colour, time);
}

void DogfightShip::renderAt(sf::RenderWindow* window, sf::Vector2f centre, sf::Color colour, float time) const
{
	// Flicker while the post hit grace is running
	const bool blink = graceTimer > 0.0f && std::fmod(time * 24.0f, 2.0f) < 1.0f;
	const std::uint8_t alpha = blink ? 90 : 255;
	const sf::Vector2f forward = this->getHeading();
	const sf::Vector2f side(-forward.y, forward.x);
	auto at = [&](float along, float across) { return centre + forward * (along * SCALE) + side * (across * SCALE); };

	sf::VertexArray flames(sf::PrimitiveType::Triangles);
	if (boostTimer > 0.0f)
	{
		const float length = 38.0f * (0.8f + 0.4f * std::sin(time * 90.0f));
		flames.append({ at(-8.0f, -6.0f), sf::Color(150, 210, 255, 230) });
		flames.append({ at(-8.0f, 6.0f), sf::Color(150, 210, 255, 230) });
		flames.append({ at(-8.0f - length, 0.0f), sf::Color(80, 140, 255, 0) });
	}
	else if (controls.thrust > 0.05f)
	{
		const float length = (6.0f + 16.0f * controls.thrust) * (0.8f + 0.4f * std::sin(time * 70.0f));
		flames.append({ at(-8.0f, -4.0f), sf::Color(255, 160, 60, 220) });
		flames.append({ at(-8.0f, 4.0f), sf::Color(255, 160, 60, 220) });
		flames.append({ at(-8.0f - length, 0.0f), sf::Color(255, 90, 30, 0) });
	}
	window->draw(flames);

	// Dark filled hull with a bright outline
	const sf::Color fill = shade(colour, 0.35f, alpha);
	const sf::Color edge = sf::Color(colour.r, colour.g, colour.b, alpha);
	sf::VertexArray hull(sf::PrimitiveType::Triangles);
	hull.append({ at(16.0f, 0.0f), fill });
	hull.append({ at(-11.0f, -10.0f), fill });
	hull.append({ at(-5.0f, 0.0f), fill });
	hull.append({ at(16.0f, 0.0f), fill });
	hull.append({ at(-5.0f, 0.0f), fill });
	hull.append({ at(-11.0f, 10.0f), fill });
	theme::appendLine(hull, at(16.0f, 0.0f), at(-11.0f, -10.0f), 1.5f, edge);
	theme::appendLine(hull, at(-11.0f, -10.0f), at(-5.0f, 0.0f), 1.5f, edge);
	theme::appendLine(hull, at(-5.0f, 0.0f), at(-11.0f, 10.0f), 1.5f, edge);
	theme::appendLine(hull, at(-11.0f, 10.0f), at(16.0f, 0.0f), 1.5f, edge);
	theme::appendDisc(hull, at(3.0f, 0.0f), 2.0f, edge, 8);
	window->draw(hull);

	// Shield bubble fades as it runs out
	if (shieldTimer > 0.0f)
	{
		const float remaining = this->getShieldRemaining();
		const float pulse = 0.5f + 0.5f * std::sin(time * 30.0f);
		theme::drawRing(window, centre, (26.0f + 2.0f * pulse) * SCALE, 1.5f, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, theme::toAlpha(120.0f + 135.0f * remaining)), sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, theme::toAlpha(20.0f + 40.0f * remaining)));
	}

	// Health bar above the ship, kept level
	const sf::Vector2f barPos(centre.x - HEALTH_BAR_WIDTH / 2.0f, centre.y - DogfightShip::RADIUS * 2.6f);
	const float fraction = this->getHealthFraction();
	const sf::Color barColour = fraction > 0.5f ? theme::ELITE : (fraction > 0.25f ? theme::WARNING : theme::NEGATIVE);
	theme::drawRect(window, barPos, { HEALTH_BAR_WIDTH, 2.0f }, sf::Color(255, 255, 255, 36));
	theme::drawRect(window, barPos, { HEALTH_BAR_WIDTH * fraction, 2.0f }, barColour);
}

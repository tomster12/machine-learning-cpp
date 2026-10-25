#include "stdafx.h"
#include "DogfightArena.h"
#include "UITheme.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float SPAWN_CLEARANCE = 300.0f;
	constexpr size_t MAX_WAVE_SIZE = 120;
	constexpr size_t MAX_ASTEROIDS = 250;

	sf::Vector2f rotate(sf::Vector2f v, float angle)
	{
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		return { v.x * c - v.y * s, v.x * s + v.y * c };
	}
}

DogfightArena::DogfightArena(std::shared_ptr<const DogfightLevel> level)
	: level(std::move(level)), rng(this->level->getSeed())
{
	// The player starts on the left facing right, the rival on the right facing left, each knocked slightly off true by the level
	ships[0] = DogfightShip({ DogfightLevel::WIDTH * 0.25f + random(-200.0f, 200.0f), DogfightLevel::HEIGHT * 0.5f + random(-300.0f, 300.0f) }, random(-0.4f, 0.4f));
	ships[1] = DogfightShip({ DogfightLevel::WIDTH * 0.75f + random(-200.0f, 200.0f), DogfightLevel::HEIGHT * 0.5f + random(-300.0f, 300.0f) }, PI + random(-0.4f, 0.4f));
	this->spawnWave();
}

DogfightArena::Asteroid DogfightArena::makeAsteroid(sf::Vector2f pos, sf::Vector2f vel, int size)
{
	Asteroid asteroid;
	asteroid.pos = DogfightLevel::wrapPosition(pos);
	asteroid.vel = vel;
	asteroid.size = size;
	asteroid.rot = random(0.0f, 2.0f * PI);
	asteroid.spin = random(-1.2f, 1.2f);
	for (float& radius : asteroid.shape) radius = random(0.72f, 1.15f);
	return asteroid;
}

void DogfightArena::spawnWave()
{
	// Each wave brings more rocks than the last, a third of the first wave dropped within reach of a ship so the rocks get involved early
	const size_t count = std::min(level->getAsteroidCount() + static_cast<size_t>(wave) * 5, MAX_WAVE_SIZE);
	const bool first = wave == 0;
	wave++;
	for (size_t i = 0; i < count; i++)
	{
		sf::Vector2f pos;
		for (int attempt = 0; attempt < 20; attempt++)
		{
			pos = { random(0.0f, DogfightLevel::WIDTH), random(0.0f, DogfightLevel::HEIGHT) };
			if (first && i % 3 == 0)
			{
				const float around = random(0.0f, 2.0f * PI);
				pos = DogfightLevel::wrapPosition(ships[i % 2].getPosition() + sf::Vector2f(std::cos(around), std::sin(around)) * random(SPAWN_CLEARANCE + 50.0f, 900.0f));
			}
			const bool clear = DogfightLevel::wrapDelta(ships[0].getPosition(), pos).length() > SPAWN_CLEARANCE && DogfightLevel::wrapDelta(ships[1].getPosition(), pos).length() > SPAWN_CLEARANCE;
			if (clear) break;
		}

		const float angle = random(0.0f, 2.0f * PI);
		const float speed = random(0.5f, 1.0f) * level->getAsteroidSpeed();
		asteroids.push_back(this->makeAsteroid(pos, sf::Vector2f(std::cos(angle), std::sin(angle)) * speed, 3));
	}
}

void DogfightArena::step(const DogfightShip::Controls& playerControls, const DogfightShip::Controls& rivalControls)
{
	const DogfightShip::Controls controls[2] = { playerControls, rivalControls };
	for (size_t i = 0; i < 2; i++)
	{
		DogfightShip& ship = ships[i];
		if (!ship.isAlive()) continue;

		ship.control(controls[i], level->isBoostEnabled(), level->isShieldEnabled());
		if (ship.consumeBoostStart()) this->addEvent(EventType::BOOST_STARTED, ship.getPosition(), 0, i);
		if (ship.consumeShieldStart()) this->addEvent(EventType::SHIELD_STARTED, ship.getPosition(), 0, i);

		if (ship.takeShot())
		{
			const sf::Vector2f heading = ship.getHeading();
			bullets.push_back({ DogfightLevel::wrapPosition(ship.getPosition() + heading * (DogfightShip::RADIUS + 4.0f)), heading * DogfightShip::BULLET_SPEED + ship.getVelocity(), DogfightShip::BULLET_LIFE, i, false });
			tallies[i].shots++;
		}
		ship.step(TIME_STEP);
	}

	this->moveObjects();
	this->collideBullets();
	this->collideShips();
	if (asteroids.empty() && !this->isOver()) this->spawnWave();
}

void DogfightArena::moveObjects()
{
	for (Asteroid& asteroid : asteroids)
	{
		asteroid.pos = DogfightLevel::wrapPosition(asteroid.pos + asteroid.vel * TIME_STEP);
		asteroid.rot += asteroid.spin * TIME_STEP;
	}

	// A bullet that runs out of life without hitting anything counts as a miss
	for (Bullet& bullet : bullets)
	{
		bullet.pos = DogfightLevel::wrapPosition(bullet.pos + bullet.vel * TIME_STEP);
		bullet.life -= TIME_STEP;
		if (bullet.life <= 0.0f && !bullet.spent)
		{
			bullet.spent = true;
			tallies[bullet.owner].missed++;
		}
	}
}

void DogfightArena::collideBullets()
{
	std::vector<Asteroid> fragments;
	for (Bullet& bullet : bullets)
	{
		if (bullet.spent) continue;

		for (size_t i = 0; i < 2 && !bullet.spent; i++)
		{
			const DogfightShip& target = ships[i];
			if (i == bullet.owner || !target.isAlive()) continue;
			if (DogfightLevel::wrapDelta(target.getPosition(), bullet.pos).length() > DogfightShip::RADIUS + DogfightShip::BULLET_RADIUS) continue;

			// A shield or a hit grace soaks the bullet without any damage
			const float applied = ships[i].takeDamage(BULLET_DAMAGE);
			tallies[bullet.owner].damageDealt += applied;
			tallies[bullet.owner].hits++;
			tallies[i].damageTaken += applied;
			this->addEvent(applied > 0.0f ? EventType::SHIP_HIT : EventType::SHIP_BLOCKED, bullet.pos, 0, i);
			if (!ships[i].isAlive()) this->addEvent(EventType::SHIP_DESTROYED, ships[i].getPosition(), 0, i);
			bullet.spent = true;
		}

		for (Asteroid& asteroid : asteroids)
		{
			if (bullet.spent) break;
			if (asteroid.size == 0) continue;
			if (DogfightLevel::wrapDelta(asteroid.pos, bullet.pos).length() > asteroid.getRadius() + DogfightShip::BULLET_RADIUS) continue;

			tallies[bullet.owner].rockPoints += asteroid.getValue();
			tallies[bullet.owner].rocks++;
			tallies[bullet.owner].hits++;
			this->destroyAsteroid(asteroid, bullet.vel, fragments);
			bullet.spent = true;
		}
	}

	std::erase_if(bullets, [](const Bullet& bullet) { return bullet.spent; });
	std::erase_if(asteroids, [](const Asteroid& asteroid) { return asteroid.size == 0; });
	for (Asteroid& fragment : fragments)
	{
		if (asteroids.size() < MAX_ASTEROIDS) asteroids.push_back(fragment);
	}
}

void DogfightArena::collideShips()
{
	std::vector<Asteroid> fragments;
	for (size_t i = 0; i < 2; i++)
	{
		DogfightShip& ship = ships[i];
		if (!ship.isAlive()) continue;

		for (Asteroid& asteroid : asteroids)
		{
			if (asteroid.size == 0) continue;

			const sf::Vector2f delta = DogfightLevel::wrapDelta(ship.getPosition(), asteroid.pos);
			const float reach = asteroid.getRadius() + DogfightShip::RADIUS;
			const float distance = delta.length();
			if (distance > reach) continue;

			if (ship.isShielded())
			{
				// A shield throws rocks off without breaking them
				const sf::Vector2f normal = distance > 0.001f ? delta / distance : sf::Vector2f(1.0f, 0.0f);
				const sf::Vector2f relative = asteroid.vel - ship.getVelocity();
				const float closing = relative.x * normal.x + relative.y * normal.y;
				if (closing < 0.0f) asteroid.vel -= normal * (2.0f * closing);
				asteroid.pos = DogfightLevel::wrapPosition(ship.getPosition() + normal * reach);
				this->addEvent(EventType::SHIP_BLOCKED, ship.getPosition() + normal * DogfightShip::RADIUS, 0, i);
				continue;
			}

			// Ramming a rock wrecks it, with no credit, and costs health
			const float applied = ship.takeDamage(asteroid.getDamage());
			tallies[i].damageTaken += applied;
			if (applied > 0.0f) this->addEvent(EventType::SHIP_HIT, ship.getPosition(), 0, i);
			if (!ship.isAlive()) this->addEvent(EventType::SHIP_DESTROYED, ship.getPosition(), 0, i);
			this->destroyAsteroid(asteroid, ship.getVelocity(), fragments);
		}
	}

	std::erase_if(asteroids, [](const Asteroid& asteroid) { return asteroid.size == 0; });
	for (Asteroid& fragment : fragments)
	{
		if (asteroids.size() < MAX_ASTEROIDS) asteroids.push_back(fragment);
	}

	// Ships ramming each other trade damage and bounce apart
	if (!ships[0].isAlive() || !ships[1].isAlive()) return;

	const sf::Vector2f between = DogfightLevel::wrapDelta(ships[0].getPosition(), ships[1].getPosition());
	const float gap = between.length();
	if (gap > DogfightShip::RADIUS * 2.0f) return;

	const sf::Vector2f normal = gap > 0.001f ? between / gap : sf::Vector2f(1.0f, 0.0f);
	const float overlap = DogfightShip::RADIUS * 2.0f - gap;
	const sf::Vector2f relative = ships[1].getVelocity() - ships[0].getVelocity();
	const float closing = relative.x * normal.x + relative.y * normal.y;
	const sf::Vector2f impulse = closing < 0.0f ? normal * closing : sf::Vector2f(0.0f, 0.0f);
	ships[0].nudge(-normal * (overlap / 2.0f), impulse);
	ships[1].nudge(normal * (overlap / 2.0f), -impulse);

	for (size_t i = 0; i < 2; i++)
	{
		const float applied = ships[i].takeDamage(RAM_DAMAGE);
		tallies[i].damageTaken += applied;
		tallies[1 - i].damageDealt += applied;
		if (applied > 0.0f) this->addEvent(EventType::SHIP_HIT, ships[i].getPosition(), 0, i);
		if (!ships[i].isAlive()) this->addEvent(EventType::SHIP_DESTROYED, ships[i].getPosition(), 0, i);
	}
}

void DogfightArena::destroyAsteroid(Asteroid& asteroid, sf::Vector2f impulse, std::vector<Asteroid>& fragments)
{
	const int size = asteroid.size;
	asteroid.size = 0;
	this->addEvent(EventType::ROCK_DESTROYED, asteroid.pos, size, 0);
	if (size <= 1) return;

	// Two smaller rocks fly off either side of the original's path, a little faster
	const float childRadius = size == 3 ? 34.0f : 13.0f;
	const float offsetAngle = random(0.0f, 2.0f * PI);
	const sf::Vector2f offset = sf::Vector2f(std::cos(offsetAngle), std::sin(offsetAngle)) * (childRadius * 0.6f);
	for (int side = -1; side <= 1; side += 2)
	{
		sf::Vector2f vel = rotate(asteroid.vel, random(0.35f, 0.9f) * static_cast<float>(side)) * 1.15f + impulse * 0.04f;
		if (vel.length() < 15.0f) vel = sf::Vector2f(std::cos(offsetAngle), std::sin(offsetAngle)) * 15.0f * static_cast<float>(side);
		fragments.push_back(this->makeAsteroid(asteroid.pos + offset * static_cast<float>(side), vel, size - 1));
	}
}

void DogfightArena::addEvent(EventType type, sf::Vector2f pos, int size, size_t owner)
{
	// Only the first few matter, nobody may be watching this arena
	if (events.size() < MAX_EVENTS) events.push_back({ type, pos, size, owner });
}

std::vector<DogfightArena::Event> DogfightArena::takeEvents()
{
	std::vector<Event> taken;
	taken.swap(events);
	return taken;
}

void DogfightArena::render(sf::RenderWindow* window, sf::Color playerColour, sf::Color rivalColour, float time) const
{
	sf::VertexArray vertices(sf::PrimitiveType::Triangles);

	for (const Asteroid& asteroid : asteroids)
	{
		const sf::Color fill(44 + asteroid.size * 4, 48 + asteroid.size * 4, 62 + asteroid.size * 5, 235);
		const sf::Color edge(150, 160, 186, 230);
		for (const sf::Vector2f& offset : DogfightLevel::getImageOffsets(asteroid.pos, asteroid.getRadius() * 1.2f))
		{
			// Rough polygon, filled as a fan and outlined
			sf::Vector2f points[9];
			for (size_t k = 0; k < 9; k++)
			{
				const float angle = asteroid.rot + static_cast<float>(k) * (2.0f * PI / 9.0f);
				points[k] = asteroid.pos + offset + sf::Vector2f(std::cos(angle), std::sin(angle)) * (asteroid.getRadius() * asteroid.shape[k]);
			}
			for (size_t k = 0; k < 9; k++)
			{
				const sf::Vector2f next = points[(k + 1) % 9];
				vertices.append({ asteroid.pos + offset, fill });
				vertices.append({ points[k], fill });
				vertices.append({ next, fill });
				theme::appendLine(vertices, points[k], next, 3.0f, edge);
			}
		}
	}

	// Bullets with a short streak and a soft glow
	for (const Bullet& bullet : bullets)
	{
		const sf::Color colour = bullet.owner == 0 ? playerColour : rivalColour;
		theme::appendLine(vertices, bullet.pos - bullet.vel * 0.04f, bullet.pos, 5.0f, sf::Color(colour.r, colour.g, colour.b, 120));
		theme::appendDisc(vertices, bullet.pos, 18.0f, sf::Color(colour.r, colour.g, colour.b, 45), 10);
		theme::appendDisc(vertices, bullet.pos, DogfightShip::BULLET_RADIUS, sf::Color(255, 255, 255, 255), 10);
	}
	window->draw(vertices);

	ships[1].render(window, rivalColour, time);
	ships[0].render(window, playerColour, time);
}

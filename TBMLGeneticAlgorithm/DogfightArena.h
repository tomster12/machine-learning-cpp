#pragma once

#include <array>
#include <random>
#include "DogfightLevel.h"
#include "DogfightShip.h"

class DogfightArena
{
public:
	static constexpr float TIME_STEP = 1.0f / 60.0f;
	static constexpr size_t MAX_EVENTS = 48;
	static constexpr float BULLET_DAMAGE = 20.0f;
	static constexpr float RAM_DAMAGE = 25.0f;

	enum class EventType { ROCK_DESTROYED, SHIP_HIT, SHIP_BLOCKED, SHIP_DESTROYED, BOOST_STARTED, SHIELD_STARTED };

	struct Asteroid
	{
		sf::Vector2f pos;
		sf::Vector2f vel;
		float rot = 0.0f;
		float spin = 0.0f;
		int size = 3;
		std::array<float, 9> shape = {};

		float getRadius() const { return size >= 3 ? 80.0f : (size == 2 ? 34.0f : 13.0f); }
		float getValue() const { return size >= 3 ? 50.0f : (size == 2 ? 125.0f : 250.0f); }
		float getDamage() const { return size >= 3 ? 32.0f : (size == 2 ? 20.0f : 10.0f); }
	};

	struct Bullet
	{
		sf::Vector2f pos;
		sf::Vector2f vel;
		float life = 0.0f;
		size_t owner = 0;
		bool spent = false;
	};

	struct Event
	{
		EventType type = EventType::ROCK_DESTROYED;
		sf::Vector2f pos;
		int size = 0;
		size_t owner = 0;
	};

	struct Tally
	{
		float rockPoints = 0.0f;
		float damageDealt = 0.0f;
		float damageTaken = 0.0f;
		int shots = 0;
		int hits = 0;
		int missed = 0;
		int rocks = 0;
	};

	explicit DogfightArena(std::shared_ptr<const DogfightLevel> level);

	void step(const DogfightShip::Controls& playerControls, const DogfightShip::Controls& rivalControls);
	std::vector<Event> takeEvents();
	void render(sf::RenderWindow* window, sf::Color playerColour, sf::Color rivalColour, float time) const;

	const DogfightLevel& getLevel() const { return *level; }
	const DogfightShip& getShip(size_t index) const { return ships[index]; }
	const std::vector<Asteroid>& getAsteroids() const { return asteroids; }
	const std::vector<Bullet>& getBullets() const { return bullets; }
	const Tally& getTally(size_t index) const { return tallies[index]; }
	int getWave() const { return wave; }
	bool isOver() const { return !ships[0].isAlive() || !ships[1].isAlive(); }

private:
	float random(float low, float high) { return std::uniform_real_distribution<float>(low, high)(rng); }
	Asteroid makeAsteroid(sf::Vector2f pos, sf::Vector2f vel, int size);
	void spawnWave();
	void destroyAsteroid(Asteroid& asteroid, sf::Vector2f impulse, std::vector<Asteroid>& fragments);
	void addEvent(EventType type, sf::Vector2f pos, int size, size_t owner);
	void moveObjects();
	void collideBullets();
	void collideShips();

	std::shared_ptr<const DogfightLevel> level;
	std::mt19937 rng;
	std::array<DogfightShip, 2> ships;
	std::array<Tally, 2> tallies;
	std::vector<Asteroid> asteroids;
	std::vector<Bullet> bullets;
	std::vector<Event> events;
	int wave = 0;
};

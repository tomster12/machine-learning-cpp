#pragma once

class DogfightLevel
{
public:
	static constexpr float WIDTH = 3500.0f;
	static constexpr float HEIGHT = 2500.0f;
	static constexpr int RAMP_LEVELS = 150;
	static constexpr int BOOST_LEVEL = 12;
	static constexpr int SHIELD_LEVEL = 25;

	explicit DogfightLevel(int number);

	static float getGenerationDifficulty(int number);
	static sf::Vector2f wrapPosition(sf::Vector2f pos);
	static sf::Vector2f wrapDelta(sf::Vector2f from, sf::Vector2f to);
	static std::vector<sf::Vector2f> getImageOffsets(sf::Vector2f pos, float radius);

	int getNumber() const { return number; }
	float getGenerationDifficulty() const { return difficulty; }
	size_t getAsteroidCount() const { return asteroidCount; }
	float getAsteroidSpeed() const { return asteroidSpeed; }
	bool isBoostEnabled() const { return number >= BOOST_LEVEL; }
	bool isShieldEnabled() const { return number >= SHIELD_LEVEL; }
	unsigned int getSeed() const { return seed; }

private:
	int number = 0;
	float difficulty = 0.0f;
	size_t asteroidCount = 0;
	float asteroidSpeed = 0.0f;
	unsigned int seed = 0;
};

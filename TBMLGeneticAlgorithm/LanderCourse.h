#pragma once

class LanderCourse
{
public:
	static constexpr float WIDTH = 1400.0f;
	static constexpr float HEIGHT = 1000.0f;
	static constexpr float POINT_SPACING = 20.0f;
	static constexpr float RAY_STEP = 10.0f;
	static constexpr int RAMP_GENERATIONS = 80;

	explicit LanderCourse(int number);

	static float getGenerationDifficulty(int number);

	float getHeight(float x) const;
	float castRay(sf::Vector2f start, float angle, float length) const;

	int getNumber() const { return number; }
	float getGenerationDifficulty() const { return difficulty; }
	float getPadWidth() const { return padWidth; }
	sf::Vector2f getPadCentre() const { return { padX, padY }; }
	sf::Vector2f getSpawnPos() const { return spawnPos; }
	sf::Vector2f getSpawnVel() const { return spawnVel; }
	float getSpawnRot() const { return spawnRot; }
	const std::vector<float>& getHeights() const { return heights; }

private:
	int number = 0;
	float difficulty = 0.0f;
	float padWidth = 0.0f;
	float padX = 0.0f;
	float padY = 0.0f;
	sf::Vector2f spawnPos;
	sf::Vector2f spawnVel;
	float spawnRot = 0.0f;
	std::vector<float> heights;
};

#pragma once

class WalkerCourse
{
public:
	static constexpr float WIDTH = 1400.0f;
	static constexpr float HEIGHT = 1000.0f;
	static constexpr float POINT_SPACING = 20.0f;
	static constexpr float START_X = 100.0f;
	static constexpr float GOAL_X = 1300.0f;
	static constexpr float BASE_HEIGHT = 780.0f;
	static constexpr int RAMP_GENERATIONS = 100;

	explicit WalkerCourse(int number);

	static float getDifficultyFor(int number);

	float getHeight(float x) const;

	int getNumber() const { return number; }
	float getDifficulty() const { return difficulty; }
	float getRoughness() const { return roughness; }
	const std::vector<float>& getHeights() const { return heights; }

private:
	int number = 0;
	float difficulty = 0.0f;
	float roughness = 0.0f;
	std::vector<float> heights;
};

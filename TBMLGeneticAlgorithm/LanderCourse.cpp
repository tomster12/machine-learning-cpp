#include "stdafx.h"
#include <random>
#include "LanderCourse.h"

LanderCourse::LanderCourse(int number)
	: number(number)
{
	std::mt19937 rng(static_cast<unsigned int>(number) * 7919u + 17u);
	auto random = [&](float low, float high) { return low + (high - low) * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng); };
	auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

	// Later courses have a narrower pad, rougher ground and a wilder spawn
	difficulty = getGenerationDifficulty(number);
	padWidth = lerp(280.0f, 130.0f, difficulty);
	const float roughness = lerp(30.0f, 170.0f, difficulty);

	// Rolling terrain from three sines
	const float frequencies[3] = { random(0.003f, 0.006f), random(0.008f, 0.014f), random(0.02f, 0.035f) };
	const float phases[3] = { random(0.0f, 6.28f), random(0.0f, 6.28f), random(0.0f, 6.28f) };
	const float weights[3] = { 0.6f, 0.3f, 0.1f };
	const size_t count = static_cast<size_t>(WIDTH / POINT_SPACING) + 1;
	heights.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		const float x = static_cast<float>(i) * POINT_SPACING;
		float offset = 0.0f;
		for (int k = 0; k < 3; k++) offset += weights[k] * std::sin(x * frequencies[k] + phases[k]);
		heights[i] = std::clamp(800.0f + roughness * offset, 560.0f, 930.0f);
	}

	// Flatten the ground around the pad, easing back into the terrain either side
	padX = random(padWidth / 2.0f + 100.0f, WIDTH - padWidth / 2.0f - 100.0f);
	padY = getHeight(padX);
	for (size_t i = 0; i < count; i++)
	{
		const float dist = std::abs(static_cast<float>(i) * POINT_SPACING - padX);
		const float blend = std::clamp(1.0f - (dist - padWidth / 2.0f) / 60.0f, 0.0f, 1.0f);
		heights[i] = lerp(heights[i], padY, blend);
	}

	// Early courses drop the ship straight onto the pad from low down, later ones start high, off to the side and tumbling
	spawnPos = { std::clamp(padX + random(-1.0f, 1.0f) * lerp(0.0f, 600.0f, difficulty), 150.0f, WIDTH - 150.0f), lerp(420.0f, 90.0f, difficulty) };
	spawnVel = { random(-1.0f, 1.0f) * lerp(0.0f, 110.0f, difficulty), random(0.0f, 1.0f) * lerp(0.0f, 40.0f, difficulty) };
	spawnRot = random(-1.0f, 1.0f) * lerp(0.0f, 0.8f, difficulty);
}

float LanderCourse::getGenerationDifficulty(int number)
{
	return std::min(1.0f, static_cast<float>(number - 1) / static_cast<float>(RAMP_GENERATIONS));
}

float LanderCourse::getHeight(float x) const
{
	const float t = std::clamp(x, 0.0f, WIDTH) / POINT_SPACING;
	const size_t i = std::min(static_cast<size_t>(t), heights.size() - 2);
	const float fraction = t - static_cast<float>(i);
	return heights[i] + (heights[i + 1] - heights[i]) * fraction;
}

float LanderCourse::castRay(sf::Vector2f start, float angle, float length) const
{
	// March along the ray, returning the fraction of its length at the first hit
	const sf::Vector2f step(std::cos(angle) * RAY_STEP, std::sin(angle) * RAY_STEP);
	const int steps = static_cast<int>(length / RAY_STEP);
	sf::Vector2f point = start;
	for (int i = 1; i <= steps; i++)
	{
		point += step;
		if (point.y >= getHeight(point.x)) return static_cast<float>(i) * RAY_STEP / length;
	}
	return 1.0f;
}

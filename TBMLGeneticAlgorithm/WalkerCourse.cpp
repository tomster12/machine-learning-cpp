#include "stdafx.h"
#include <random>
#include "WalkerCourse.h"

WalkerCourse::WalkerCourse(int number)
	: number(number)
{
	std::mt19937 rng(static_cast<unsigned int>(number) * 7919u + 17u);
	auto random = [&](float low, float high) { return low + (high - low) * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng); };
	auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

	// Early courses are flat, later ones roll and bump
	difficulty = getGenerationDifficulty(number);
	roughness = lerp(0.0f, 70.0f, difficulty);

	const float frequencies[3] = { random(0.004f, 0.007f), random(0.01f, 0.016f), random(0.025f, 0.04f) };
	const float phases[3] = { random(0.0f, 6.28f), random(0.0f, 6.28f), random(0.0f, 6.28f) };
	const float weights[3] = { 0.55f, 0.3f, 0.15f };
	const size_t count = static_cast<size_t>(WIDTH / POINT_SPACING) + 1;
	heights.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		const float x = static_cast<float>(i) * POINT_SPACING;
		float offset = 0.0f;
		for (int k = 0; k < 3; k++) offset += weights[k] * std::sin(x * frequencies[k] + phases[k]);

		// Ease in from a flat start so every walker begins on level ground
		const float ease = std::clamp((x - START_X) / 160.0f, 0.0f, 1.0f);
		heights[i] = BASE_HEIGHT + roughness * offset * ease;
	}
}

float WalkerCourse::getGenerationDifficulty(int number)
{
	return std::min(1.0f, static_cast<float>(number - 1) / static_cast<float>(RAMP_GENERATIONS));
}

float WalkerCourse::getHeight(float x) const
{
	const float t = std::clamp(x, 0.0f, WIDTH) / POINT_SPACING;
	const size_t i = std::min(static_cast<size_t>(t), heights.size() - 2);
	const float fraction = t - static_cast<float>(i);
	return heights[i] + (heights[i + 1] - heights[i]) * fraction;
}

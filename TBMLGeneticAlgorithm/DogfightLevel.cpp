#include "stdafx.h"
#include "DogfightLevel.h"

DogfightLevel::DogfightLevel(int number)
	: number(number)
{
	// Later levels throw more and faster rocks
	difficulty = getGenerationDifficulty(number);
	asteroidCount = static_cast<size_t>(std::lerp(30.0f, 60.0f, difficulty) + 0.5f);
	asteroidSpeed = std::lerp(45.0f, 110.0f, difficulty);
	seed = static_cast<unsigned int>(number) * 7919u + 17u;
}

float DogfightLevel::getGenerationDifficulty(int number)
{
	return std::min(1.0f, static_cast<float>(number - 1) / static_cast<float>(RAMP_LEVELS));
}

sf::Vector2f DogfightLevel::wrapPosition(sf::Vector2f pos)
{
	pos.x = std::fmod(pos.x, WIDTH);
	pos.y = std::fmod(pos.y, HEIGHT);
	if (pos.x < 0.0f) pos.x += WIDTH;
	if (pos.y < 0.0f) pos.y += HEIGHT;
	return pos;
}

sf::Vector2f DogfightLevel::wrapDelta(sf::Vector2f from, sf::Vector2f to)
{
	// Shortest way from one point to another across the wrapped edges
	sf::Vector2f delta = to - from;
	if (delta.x > WIDTH / 2.0f) delta.x -= WIDTH;
	else if (delta.x < -WIDTH / 2.0f) delta.x += WIDTH;
	if (delta.y > HEIGHT / 2.0f) delta.y -= HEIGHT;
	else if (delta.y < -HEIGHT / 2.0f) delta.y += HEIGHT;
	return delta;
}

std::vector<sf::Vector2f> DogfightLevel::getImageOffsets(sf::Vector2f pos, float radius)
{
	// Offsets that draw a copy of something wherever it pokes over an edge, the real one first
	std::vector<sf::Vector2f> offsets;
	const float xs[3] = { 0.0f, -WIDTH, WIDTH };
	const float ys[3] = { 0.0f, -HEIGHT, HEIGHT };
	for (float x : xs)
	{
		for (float y : ys)
		{
			const sf::Vector2f image = pos + sf::Vector2f(x, y);
			const bool visible = image.x + radius >= 0.0f && image.x - radius <= WIDTH && image.y + radius >= 0.0f && image.y - radius <= HEIGHT;
			if (visible) offsets.push_back({ x, y });
		}
	}
	return offsets;
}

#pragma once

#include <deque>

class Trail
{
public:
	Trail(size_t maxLength, float minSpacing, std::uint8_t maxAlpha = 110);

	bool follow(size_t index, int generation, sf::Vector2f pos);
	void render(sf::RenderWindow* window) const;

private:
	std::deque<sf::Vector2f> points;
	size_t maxLength = 0;
	float minSpacingSq = 0.0f;
	std::uint8_t maxAlpha = 0;
	size_t trackedIndex = 0;
	int trackedGeneration = -1;
};

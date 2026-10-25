#include "stdafx.h"
#include "Trail.h"
#include "UITheme.h"

Trail::Trail(size_t maxLength, float minSpacing, std::uint8_t maxAlpha)
	: maxLength(maxLength), minSpacingSq(minSpacing * minSpacing), maxAlpha(maxAlpha)
{}

bool Trail::follow(size_t index, int generation, sf::Vector2f pos)
{
	// Start a fresh trail whenever the followed agent changes
	const bool restarted = index != trackedIndex || generation != trackedGeneration;
	if (restarted)
	{
		points.clear();
		trackedIndex = index;
		trackedGeneration = generation;
	}

	if (points.empty() || (pos - points.back()).lengthSquared() > minSpacingSq) points.push_back(pos);
	while (points.size() > maxLength) points.pop_front();
	return restarted;
}

void Trail::render(sf::RenderWindow* window) const
{
	if (points.size() < 2) return;

	sf::VertexArray line(sf::PrimitiveType::LineStrip);
	for (size_t i = 0; i < points.size(); i++)
	{
		const float t = static_cast<float>(i) / static_cast<float>(points.size() - 1);
		line.append(sf::Vertex{ points[i], sf::Color(255, 255, 255, theme::toAlpha(maxAlpha * t)) });
	}
	window->draw(line);
}

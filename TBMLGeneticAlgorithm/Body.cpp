#include "stdafx.h"
#include "Body.h"

void Body::updateShape(sf::RectangleShape& shape) const
{
	shape.setPosition(pos);
	shape.setRotation(sf::radians(rot));
}

void Body::recalculateVertices()
{
	float c = std::cos(rot);
	float s = std::sin(rot);
	float w = size.x / 2.0f;
	float h = size.y / 2.0f;
	vertices = {
		{ pos.x + w * c - h * s, pos.y + w * s + h * c },
		{ pos.x - w * c - h * s, pos.y - w * s + h * c },
		{ pos.x - w * c + h * s, pos.y - w * s - h * c },
		{ pos.x + w * c + h * s, pos.y + w * s - h * c }
	};
}

std::pair<float, float> Body::projectVerticesOnAxis(const std::vector<sf::Vector2f>& vertices, const sf::Vector2f& axis)
{
	// Find min and max projection of all vertices on axis
	float min = (vertices[0].x * axis.x + vertices[0].y * axis.y);
	float max = min;
	for (const auto& vertex : vertices)
	{
		float projection = (vertex.x * axis.x + vertex.y * axis.y);
		if (projection < min) min = projection;
		if (projection > max) max = projection;
	}
	return { min, max };
}

bool Body::overlapOnAxis(const std::vector<sf::Vector2f>& vertices1, const std::vector<sf::Vector2f>& vertices2, const sf::Vector2f& axis)
{
	// Overlap on axis if projection min and max overlap
	auto pair1 = projectVerticesOnAxis(vertices1, axis);
	auto pair2 = projectVerticesOnAxis(vertices2, axis);
	return !(pair1.second < pair2.first || pair2.second < pair1.first);
}

bool Body::intersectBody(const Body& other) const
{
	// Get all axes as all edges from both rectangles
	sf::Vector2f axes[8];
	for (size_t i = 0; i < 4; ++i)
	{
		sf::Vector2f edge1 = vertices[i] - vertices[(i + 1) % 4];
		sf::Vector2f edge2 = other.vertices[i] - other.vertices[(i + 1) % 4];
		axes[i * 2 + 0] = sf::Vector2f(-edge1.y, edge1.x);
		axes[i * 2 + 1] = sf::Vector2f(-edge2.y, edge2.x);
	}

	// Collision if there is no axis of separation
	for (const auto& axis : axes)
	{
		if (!overlapOnAxis(vertices, other.vertices, axis)) return false;
	}
	return true;
}

float Body::raycast(sf::Vector2f start, sf::Vector2f end) const
{
	// Nearest edge the segment crosses, as a fraction of the way along it, 1 if it crosses none
	const sf::Vector2f direction = end - start;
	float nearest = 1.0f;
	for (size_t i = 0; i < 4; ++i)
	{
		const sf::Vector2f edgeStart = vertices[i];
		const sf::Vector2f edge = vertices[(i + 1) % 4] - edgeStart;
		const float denominator = direction.cross(edge);
		if (std::abs(denominator) < 0.0001f) continue;

		const sf::Vector2f offset = edgeStart - start;
		const float t = offset.cross(edge) / denominator;
		const float u = offset.cross(direction) / denominator;
		if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) nearest = std::min(nearest, t);
	}
	return nearest;
}

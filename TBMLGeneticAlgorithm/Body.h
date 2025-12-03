#pragma once

class Body
{
public:
	sf::Vector2f pos;
	sf::Vector2f size;
	float rot = 0.0f;

	Body() = default;
	Body(sf::Vector2f pos, sf::Vector2f size, float rot = 0.0f) : pos(pos), size(size), rot(rot) {};

	void updateShape(sf::RectangleShape& shape) const;
	void recalculateVertices();
	bool intersectBody(const Body& other) const;
	bool intersectRaycast(sf::Vector2f start, sf::Vector2f end) const;

private:
	std::vector<sf::Vector2f> vertices;
	static bool overlapOnAxis(const std::vector<sf::Vector2f>& vertices1, const std::vector<sf::Vector2f>& vertices2, const sf::Vector2f& axis);
	static std::pair<float, float> projectVerticesOnAxis(const std::vector<sf::Vector2f>& vertices, const sf::Vector2f& axis);
};

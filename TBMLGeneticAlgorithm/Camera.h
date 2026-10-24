#pragma once

class Camera
{
public:
	bool apply(sf::RenderWindow* window, sf::Vector2f worldSize);
	void handleEvent(const sf::Event& event);

private:
	float getScale() const;
	void zoomAt(sf::Vector2f pixel, float factor);

	sf::Vector2f worldSize;
	sf::Vector2f viewportSize;
	sf::Vector2f offset;
	float zoom = 1.0f;
	bool isDragging = false;
	sf::Vector2i lastMouse;
};

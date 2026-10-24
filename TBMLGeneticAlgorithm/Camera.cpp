#include "stdafx.h"
#include "Camera.h"

bool Camera::apply(sf::RenderWindow* window, sf::Vector2f worldSize)
{
	this->worldSize = worldSize;
	viewportSize = sf::Vector2f(window->getSize());
	if (getScale() <= 0.0f) return false;

	// World is drawn centred and scaled to fit the window
	window->setView(sf::View(worldSize / 2.0f + offset, viewportSize / getScale()));
	return true;
}

void Camera::handleEvent(const sf::Event& event)
{
	// Scroll to zoom, right drag to pan, middle click to reset
	if (const auto* scrolled = event.getIf<sf::Event::MouseWheelScrolled>())
	{
		this->zoomAt(sf::Vector2f(scrolled->position), scrolled->delta > 0.0f ? 1.1f : 1.0f / 1.1f);
	}
	else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (pressed->button == sf::Mouse::Button::Right)
		{
			isDragging = true;
			lastMouse = pressed->position;
		}
		else if (pressed->button == sf::Mouse::Button::Middle)
		{
			offset = sf::Vector2f();
			zoom = 1.0f;
		}
	}
	else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
	{
		if (released->button == sf::Mouse::Button::Right) isDragging = false;
	}
	else if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		if (isDragging && getScale() > 0.0f) offset -= sf::Vector2f(moved->position - lastMouse) / getScale();
		lastMouse = moved->position;
	}
}

float Camera::getScale() const
{
	if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f || worldSize.x <= 0.0f || worldSize.y <= 0.0f) return 0.0f;
	return std::min(viewportSize.x / worldSize.x, viewportSize.y / worldSize.y) * zoom;
}

void Camera::zoomAt(sf::Vector2f pixel, float factor)
{
	if (getScale() <= 0.0f) return;

	// Keep the world point under the cursor fixed while zooming
	const sf::Vector2f fromCentre = pixel - viewportSize / 2.0f;
	const sf::Vector2f worldPoint = worldSize / 2.0f + offset + fromCentre / getScale();
	zoom = std::clamp(zoom * factor, 0.5f, 8.0f);
	offset = worldPoint - fromCentre / getScale() - worldSize / 2.0f;
}

#pragma once

#include "stdafx.h"
#include "UIElement.h"

class UIToggleButton : public UIElement
{
public:
	static const sf::Color TOGGLE_COLOR;

	UIToggleButton(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, std::string texturePath, bool initial, std::function<void(bool)> action);

	void update() override;
	void render(sf::RenderWindow* window) override;

private:
	sf::RenderWindow* window;
	sf::Vector2f pos;
	sf::Vector2f size;
	sf::Texture texture;
	sf::Sprite sprite;
	std::function<void(bool)> action;

	bool isToggled;
	bool isHovered;
	bool isPressed;
};

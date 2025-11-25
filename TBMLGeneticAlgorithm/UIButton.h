#pragma once

#include "UIElement.h"

class UIButton : public UIElement
{
public:
	static const sf::Color HOVER_COLOR;

	UIButton(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, const std::string& texturePath, std::function<void()> action);

	void update() override;
	void render(sf::RenderWindow* window) override;

private:
	sf::RenderWindow* window;
	sf::Vector2f pos;
	sf::Vector2f size;
	sf::Texture texture;
	sf::Sprite sprite;
	std::function<void()> action;

	bool isHovered;
	bool isPressed;
};

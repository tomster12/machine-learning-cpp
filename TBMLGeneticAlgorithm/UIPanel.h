#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UIPanel : public UIElement
{
public:
	UIPanel(sf::Vector2f pos, sf::Vector2f size) : pos(pos), size(size) {}

	void setHeight(float height) { size.y = height; }
	void render(sf::RenderWindow* window) override { theme::drawPanel(window, pos, size); }

private:
	sf::Vector2f pos;
	sf::Vector2f size;
};

#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UIHeader : public UIElement
{
public:
	UIHeader(sf::Vector2f pos, float width, std::string title) : pos(pos), width(width), title(std::move(title)) {}

	void render(sf::RenderWindow* window) override { theme::drawHeader(window, title, pos, width); }

private:
	sf::Vector2f pos;
	float width;
	std::string title;
};

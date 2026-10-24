#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UILabel : public UIElement
{
public:
	UILabel(sf::Vector2f pos, std::function<std::string(void)> textFunc, unsigned int size = theme::TEXT_BODY, sf::Color colour = theme::TEXT_PRIMARY, theme::Align align = theme::Align::LEFT)
		: pos(pos), textFunc(std::move(textFunc)), size(size), colour(colour), align(align) {}

	void render(sf::RenderWindow* window) override { theme::drawText(window, textFunc(), pos, size, colour, align); }

private:
	sf::Vector2f pos;
	std::function<std::string(void)> textFunc;
	unsigned int size;
	sf::Color colour;
	theme::Align align;
};

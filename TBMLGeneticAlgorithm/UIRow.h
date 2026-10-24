#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UIRow : public UIElement
{
public:
	UIRow(sf::Vector2f pos, float width, std::string label, std::function<std::string(void)> valueFunc, const sf::Color* swatch = nullptr)
		: pos(pos), width(width), label(std::move(label)), valueFunc(std::move(valueFunc)), hasSwatch(swatch != nullptr), swatch(swatch ? *swatch : sf::Color::White) {}

	void render(sf::RenderWindow* window) override
	{
		float labelX = pos.x;
		if (hasSwatch)
		{
			theme::drawRect(window, { pos.x, pos.y - 4.0f }, { 8.0f, 8.0f }, swatch);
			labelX += 16.0f;
		}
		theme::drawText(window, label, { labelX, pos.y }, theme::TEXT_BODY, theme::TEXT_SECONDARY);
		theme::drawText(window, valueFunc(), { pos.x + width, pos.y }, theme::TEXT_BODY, theme::TEXT_PRIMARY, theme::Align::RIGHT);
	}

private:
	sf::Vector2f pos;
	float width;
	std::string label;
	std::function<std::string(void)> valueFunc;
	bool hasSwatch;
	sf::Color swatch;
};

#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UITooltip : public UIElement
{
public:
	static inline std::string text;
	static inline sf::Vector2f pos;

	static void set(const std::string& tooltip, sf::Vector2f position)
	{
		text = tooltip;
		pos = position;
	}

	void render(sf::RenderWindow* window) override
	{
		if (!text.empty())
		{
			const float width = theme::measureText(text, theme::TEXT_SMALL) + 16.0f;
			const float x = std::min(pos.x, static_cast<float>(window->getSize().x) - width - theme::MARGIN);
			theme::drawPanel(window, { x, pos.y }, { width, 22.0f });
			theme::drawText(window, text, { x + 8.0f, pos.y + 11.0f }, theme::TEXT_SMALL, theme::TEXT_PRIMARY);
		}
		text.clear();
	}
};

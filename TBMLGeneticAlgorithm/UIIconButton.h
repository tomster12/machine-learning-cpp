#pragma once

#include "Icon.h"
#include "UIElement.h"
#include "UITooltip.h"

class UIIconButton : public UIElement
{
public:
	UIIconButton(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, Icon icon, Icon activeIcon, std::string tooltip, bool isToggle, std::function<void(bool)> action)
		: window(window), pos(pos), size(size), icon(icon), activeIcon(activeIcon), tooltip(std::move(tooltip)), isToggle(isToggle), action(std::move(action))
	{
		if (isToggle) this->action(isToggled);
	}

	void update() override
	{
		const sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(*window));
		isHovered = sf::FloatRect(pos, size).contains(mousePos);
		if (isHovered) UITooltip::set(tooltip, { pos.x, pos.y + size.y + 6.0f });

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		{
			if (isHovered && !isPressed)
			{
				if (isToggle) isToggled = !isToggled;
				action(isToggled);
				isPressed = true;
			}
		}
		else isPressed = false;
	}

	void render(sf::RenderWindow* window) override
	{
		this->window = window;

		sf::Color fill = isHovered ? theme::TILE_HOVER : theme::TILE_FILL;
		if (isToggled) fill = isHovered ? theme::TILE_ACTIVE_HOVER : theme::TILE_ACTIVE;
		theme::drawRect(window, pos, size, fill);

		const sf::Color colour = isToggle && !isToggled ? theme::TEXT_SECONDARY : theme::TEXT_PRIMARY;
		icons::draw(window, isToggled ? activeIcon : icon, pos + size / 2.0f, 12.0f, colour);
	}

private:
	sf::RenderWindow* window;
	sf::Vector2f pos;
	sf::Vector2f size;
	Icon icon;
	Icon activeIcon;
	std::string tooltip;
	bool isToggle;
	std::function<void(bool)> action;
	bool isToggled = false;
	bool isHovered = false;
	bool isPressed = true;
};

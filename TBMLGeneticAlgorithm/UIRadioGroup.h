#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UIRadioGroup : public UIElement
{
public:
	UIRadioGroup(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, std::vector<std::string> labels, std::function<size_t(void)> getIndex, std::function<void(size_t)> onSelect)
		: window(window), pos(pos), size(size), labels(std::move(labels)), getIndex(std::move(getIndex)), onSelect(std::move(onSelect)) {}

	void update() override
	{
		const sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(*window));
		hovered = -1;
		for (size_t i = 0; i < labels.size(); i++)
		{
			if (segmentBounds(i).contains(mousePos)) hovered = static_cast<int>(i);
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		{
			if (hovered >= 0 && !isPressed) onSelect(static_cast<size_t>(hovered));
			isPressed = true;
		}
		else isPressed = false;
	}

	void render(sf::RenderWindow* window) override
	{
		this->window = window;

		theme::drawRect(window, pos, size, theme::TILE_FILL);
		const size_t selected = getIndex();
		for (size_t i = 0; i < labels.size(); i++)
		{
			const sf::FloatRect bounds = segmentBounds(i);
			if (i == selected) theme::drawRect(window, bounds.position, bounds.size, theme::TILE_ACTIVE);
			else if (static_cast<int>(i) == hovered) theme::drawRect(window, bounds.position, bounds.size, theme::TILE_FILL);

			theme::drawText(window, labels[i], bounds.position + bounds.size / 2.0f, theme::TEXT_BODY, i == selected ? theme::TEXT_PRIMARY : theme::TEXT_SECONDARY, theme::Align::CENTER);
		}
	}

private:
	sf::FloatRect segmentBounds(size_t index) const
	{
		const float width = size.x / static_cast<float>(labels.size());
		return sf::FloatRect({ pos.x + width * static_cast<float>(index), pos.y }, { width, size.y });
	}

	sf::RenderWindow* window;
	sf::Vector2f pos;
	sf::Vector2f size;
	std::vector<std::string> labels;
	std::function<size_t(void)> getIndex;
	std::function<void(size_t)> onSelect;
	int hovered = -1;
	bool isPressed = true;
};

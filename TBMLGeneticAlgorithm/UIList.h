#pragma once

#include "UIElement.h"
#include "UITheme.h"

class UIList : public UIElement
{
public:
	UIList(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, std::vector<std::string> labels, std::function<size_t(void)> getIndex, std::function<void(size_t)> onSelect)
		: window(window), pos(pos), size(size), labels(std::move(labels)), getIndex(std::move(getIndex)), onSelect(std::move(onSelect)) {}

	void update() override
	{
		const sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(*window));
		hovered = -1;
		for (size_t i = 0; i < labels.size(); i++)
		{
			if (itemBounds(i).contains(mousePos)) hovered = static_cast<int>(i);
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

		const size_t selected = getIndex();
		for (size_t i = 0; i < labels.size(); i++)
		{
			const sf::FloatRect bounds = itemBounds(i);
			sf::Color fill = theme::TILE_FILL;
			if (i == selected) fill = static_cast<int>(i) == hovered ? theme::TILE_ACTIVE_HOVER : theme::TILE_ACTIVE;
			else if (static_cast<int>(i) == hovered) fill = theme::TILE_HOVER;
			theme::drawRect(window, bounds.position, bounds.size, fill);

			theme::drawText(window, labels[i], { bounds.position.x + theme::GAP, bounds.position.y + bounds.size.y / 2.0f }, theme::TEXT_BODY, i == selected ? theme::TEXT_PRIMARY : theme::TEXT_SECONDARY);
		}
	}

private:
	sf::FloatRect itemBounds(size_t index) const
	{
		const float height = size.y / static_cast<float>(labels.size());
		return sf::FloatRect({ pos.x, pos.y + height * static_cast<float>(index) }, { size.x, height - ITEM_GAP });
	}

	static constexpr float ITEM_GAP = 2.0f;

	sf::RenderWindow* window;
	sf::Vector2f pos;
	sf::Vector2f size;
	std::vector<std::string> labels;
	std::function<size_t(void)> getIndex;
	std::function<void(size_t)> onSelect;
	int hovered = -1;
	bool isPressed = true;
};

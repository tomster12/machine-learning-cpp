#pragma once

#include "UIElement.h"

class UIDynamicText : public UIElement
{
public:
	UIDynamicText(sf::RenderWindow* window, sf::Vector2f pos, size_t fontSize, std::function<std::string(void)> textFunc);

	void update() override {};
	void render(sf::RenderWindow* window) override;

private:
	sf::RenderWindow* window;
	sf::Vector2f pos;
	sf::Text text;
	std::function<std::string(void)> textFunc;
};

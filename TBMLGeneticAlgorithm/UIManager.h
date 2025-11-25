#pragma once

#include "UIElement.h"

class UIManager
{
public:
	void update();
	void render(sf::RenderWindow* window);

	void addElement(std::shared_ptr<UIElement>&& uiElement);

private:
	std::vector<std::shared_ptr<UIElement>> uiElements;
};

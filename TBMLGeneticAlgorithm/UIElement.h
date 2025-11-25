#pragma once

class UIElement
{
public:
	virtual void update() {};
	virtual void render(sf::RenderWindow* window) {};
};

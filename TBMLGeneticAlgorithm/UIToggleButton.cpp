#include "stdafx.h"
#include "UIToggleButton.h"
#include "UIButton.h"

const sf::Color UIToggleButton::TOGGLE_COLOR = sf::Color(140, 140, 140);

UIToggleButton::UIToggleButton(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, std::string texturePath, bool initial, std::function<void(bool)> action)
	: window(window), pos(pos), size(size), action(action), texture(texturePath), sprite(texture)
{
	sprite.setPosition({ pos.x, pos.y });
	sprite.setScale({ size.x / texture.getSize().x, size.y / texture.getSize().y });
	this->isToggled = initial;
	this->isHovered = false;
	this->isPressed = false;
	action(this->isToggled);
}

void UIToggleButton::update()
{
	sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(*this->window));
	this->isHovered = this->sprite.getGlobalBounds().contains(mousePos);

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
	{
		if (this->isHovered && !this->isPressed)
		{
			this->isToggled = !this->isToggled;
			this->action(this->isToggled);
			this->isPressed = true;
		}
	}
	else this->isPressed = false;

	if (this->isHovered) this->sprite.setColor(UIButton::HOVER_COLOR);
	else if (this->isToggled) this->sprite.setColor(UIToggleButton::TOGGLE_COLOR);
	else this->sprite.setColor(sf::Color::White);
}

void UIToggleButton::render(sf::RenderWindow* window)
{
	this->window = window;

	window->draw(sprite);
}

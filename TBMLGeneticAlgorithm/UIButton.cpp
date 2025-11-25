#include "stdafx.h"
#include "UIButton.h"

const sf::Color UIButton::HOVER_COLOR = sf::Color(100, 100, 100);

UIButton::UIButton(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, const std::string& texturePath, std::function<void()> action)
	: window(window), pos(pos), size(size), action(std::move(action)),
	isHovered(false), isPressed(false),
	texture(texturePath), sprite(texture)
{
	sprite.setPosition(pos);
	sprite.setScale({ size.x / texture.getSize().x, size.y / texture.getSize().y });
}

void UIButton::update()
{
	sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(*this->window));
	this->isHovered = this->sprite.getGlobalBounds().contains(mousePos);

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
	{
		if (this->isHovered && !this->isPressed)
		{
			this->action();
			this->isPressed = true;
		}
	}
	else this->isPressed = false;

	sprite.setColor(isHovered ? HOVER_COLOR : sf::Color::White);
}

void UIButton::render(sf::RenderWindow* window)
{
	this->window = window;
	window->draw(sprite);
}

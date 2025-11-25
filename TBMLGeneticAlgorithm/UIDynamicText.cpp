#include "stdafx.h"
#include "UIDynamicText.h"

UIDynamicText::UIDynamicText(sf::RenderWindow* window, sf::Vector2f pos, size_t fontSize, std::function<std::string(void)> textFunc)
	: window(window), pos(pos), text(global::font), textFunc(textFunc)
{
	text.setString(this->textFunc());
	text.setCharacterSize(static_cast<unsigned int>(fontSize));
	text.setPosition(pos);
}

void UIDynamicText::render(sf::RenderWindow* window)
{
	this->window = window;
	this->text.setString(this->textFunc());
	window->draw(this->text);
}

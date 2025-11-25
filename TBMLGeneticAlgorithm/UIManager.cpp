#include "stdafx.h"
#include "UIManager.h"

void UIManager::update()
{
	for (auto& uiElement : uiElements) uiElement->update();
}

void UIManager::render(sf::RenderWindow* window)
{
	for (auto& uiElement : uiElements) uiElement->render(window);
}

void UIManager::addElement(std::shared_ptr<UIElement>&& uiElement)
{
	this->uiElements.push_back(uiElement);
}

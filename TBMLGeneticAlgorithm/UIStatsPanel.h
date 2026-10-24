#pragma once

#include "UIHeader.h"
#include "UIManager.h"
#include "UIPanel.h"
#include "UIRow.h"
#include "UIRadioGroup.h"

class UIStatsPanel
{
public:
	UIStatsPanel(sf::RenderWindow* window, UIManager* ui, sf::Vector2f pos, float width)
		: window(window), ui(ui), pos(pos), width(width), cursorY(pos.y + theme::HEADER_INSET)
	{
		panel = std::make_shared<UIPanel>(pos, sf::Vector2f{ width, 0.0f });
		ui->addElement(std::shared_ptr<UIElement>(panel));
	}

	void header(const std::string& title)
	{
		if (!isFirst) cursorY += theme::GAP;
		isFirst = false;

		ui->addElement(std::make_shared<UIHeader>(sf::Vector2f{ left(), cursorY }, innerWidth(), title));
		cursorY += theme::HEADER_HEIGHT;
	}

	void row(const std::string& label, std::function<std::string(void)> valueFunc, const sf::Color* swatch = nullptr)
	{
		isFirst = false;
		ui->addElement(std::make_shared<UIRow>(sf::Vector2f{ left(), cursorY + theme::ROW_HEIGHT / 2.0f }, innerWidth(), label, std::move(valueFunc), swatch));
		cursorY += theme::ROW_HEIGHT;
	}

	void radio(std::vector<std::string> labels, std::function<size_t(void)> getIndex, std::function<void(size_t)> onSelect)
	{
		isFirst = false;
		cursorY += 6.0f;
		ui->addElement(std::make_shared<UIRadioGroup>(window, sf::Vector2f{ left(), cursorY }, sf::Vector2f{ innerWidth(), 24.0f }, std::move(labels), std::move(getIndex), std::move(onSelect)));
		cursorY += 24.0f + 6.0f;
	}

	void finish() { panel->setHeight(cursorY - pos.y + theme::PADDING); }

private:
	float left() const { return pos.x + theme::PADDING; }
	float innerWidth() const { return width - 2.0f * theme::PADDING; }

	sf::RenderWindow* window;
	UIManager* ui;
	sf::Vector2f pos;
	float width;
	float cursorY;
	bool isFirst = true;
	std::shared_ptr<UIPanel> panel;
};

#include "stdafx.h"
#include "DrawableGrid.h"

DrawableGrid::DrawableGrid()
{
	this->window = NULL;
	this->rows = 0;
	this->cols = 0;
	this->cellSize = 0;
	this->x = 0.0f;
	this->y = 0.0f;
}

DrawableGrid::DrawableGrid(sf::RenderWindow* window, int rows, int cols, int cellSize)
{
	this->window = window;
	this->rows = rows;
	this->cols = cols;
	this->cellSize = cellSize;
	this->drawRadius = 1.5f;
	this->x = 0.0f;
	this->y = 0.0f;

	this->grid = std::vector<float>(rows * cols, 0.0f);

	for (int i = 0; i < rows * cols; i++)
	{
		sf::RectangleShape cell;
		cell.setSize(sf::Vector2f((float)cellSize, (float)cellSize));
		cell.setFillColor(sf::Color::Black);
		cell.setOutlineColor(sf::Color(40, 40, 40));
		cell.setOutlineThickness(1.0f);
		this->cells.push_back(cell);
	}

	this->setPosition(0.0f, 0.0f);
}

DrawableGrid::~DrawableGrid()
{
	this->cells.clear();
}

void DrawableGrid::update()
{
	for (int i = 0; i < this->rows; i++)
	{
		for (int j = 0; j < this->cols; j++)
		{
			this->cells[i * this->cols + j].setFillColor(this->grid[i * this->cols + j] == 0.0f ? sf::Color::Black : sf::Color::White);
		}
	}

	sf::Vector2f mousePos = this->window->mapPixelToCoords(sf::Mouse::getPosition(*this->window));

	bool hasChanged = false;

	for (int i = 0; i < this->rows; i++)
	{
		for (int j = 0; j < this->cols; j++)
		{
			float dx = mousePos.x - this->x - j * this->cellSize;
			float dy = mousePos.y - this->y - i * this->cellSize;
			float distance = sqrt(dx * dx + dy * dy);

			if (distance < this->drawRadius * this->cellSize)
			{
				if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
				{
					this->grid[i * this->cols + j] = 1;
					hasChanged = true;
				}
				else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
				{
					this->grid[i * this->cols + j] = 0;
					hasChanged = true;
				}
			}
		}
	}

	if (hasChanged)
	{
		for (auto& callback : this->onGridChange) callback();
	}
}

void DrawableGrid::render()
{
	for (int i = 0; i < this->rows; i++)
	{
		for (int j = 0; j < this->cols; j++)
		{
			this->window->draw(this->cells[i * this->cols + j]);
		}
	}
}

void DrawableGrid::setPosition(float x, float y)
{
	this->x = x;
	this->y = y;

	for (int i = 0; i < this->rows; i++)
	{
		for (int j = 0; j < this->cols; j++)
		{
			this->cells[i * this->cols + j].setPosition({ this->x + j * this->cellSize, this->y + i * this->cellSize });
		}
	}
}

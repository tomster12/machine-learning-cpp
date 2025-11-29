#pragma once
#include "NeuralNetwork.h"
#include "DrawableGrid.h"

class Game
{
private:
	void update();
	void render();
	void updateGuess();

	sf::RenderWindow* window;
	sf::Clock dtClock;
	float dt;

	tbml::nn::NeuralNetwork network;
	DrawableGrid grid;
	sf::Font font;
	sf::Text guessText;
	std::vector<sf::RectangleShape> guessChances;

public:
	Game();
	~Game();
	void run();
};

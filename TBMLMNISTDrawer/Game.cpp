#include "stdafx.h"
#include "Game.h"
#include "NeuralNetwork.h"

Game::Game() : font("assets/arial.ttf"), guessText(font)
{
	this->window = NULL;
	this->dt = 0.0f;

	// Setup window using default settings
	sf::VideoMode mode({ 1400, 800 });
	window = new sf::RenderWindow(mode, "MNIST Drawer", sf::Style::Titlebar | sf::Style::Close);
	window->setFramerateLimit(120);
	window->setVerticalSyncEnabled(false);

	// Read the trained MNIST model
	network = tbml::nn::loadFromFile("../TBMLNeuralNetwork/MNIST.nn");
	network.print();
	std::cout << "Parameters: " << network.getParameterCount() << std::endl;

	// Init the grid and text
	grid = DrawableGrid(this->window, 28, 28, 400.0f / 28.0f);
	grid.setPosition(200.0f, 200.0f);

	// Initialize guess text
	this->guessText.setCharacterSize(50);
	this->guessText.setFillColor(sf::Color::White);
	this->guessText.setString("NA");
	this->guessText.setPosition({ 950.0f, 400.0f - 30.0f });

	// Initialize guess chances
	this->guessChances = std::vector<sf::RectangleShape>(10);
	float gap = 10.0f;
	float height = (400.0f - gap * 9) / 10;
	for (int i = 0; i < 10; i++)
	{
		this->guessChances[i].setSize(sf::Vector2f(100.0f, height));
		this->guessChances[i].setFillColor(sf::Color::White);
		this->guessChances[i].setOutlineColor(sf::Color::Black);
		this->guessChances[i].setOutlineThickness(1.0f);
		this->guessChances[i].setPosition({ 750.0f, 200.0f + i * (height + gap) });
	}

	// Subscribe update guess to grid change
	grid.subscribeToCellChange([&]() { this->updateGuess(); });
	this->updateGuess();
}

Game::~Game()
{
	delete this->window;
}

void Game::run()
{
	while (this->window->isOpen())
	{
		this->update();
		this->render();
	}
}

void Game::update()
{
	this->dt = this->dtClock.restart().asSeconds();

	while (const auto event = window->pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			window->close();
		}
	}

	grid.update();
}

void Game::render()
{
	window->clear();

	grid.render();
	window->draw(this->guessText);

	for (int i = 0; i < 10; i++)
	{
		window->draw(this->guessChances[i]);
	}

	window->display();
}

void Game::updateGuess()
{
	// Get the grid as tensor
	const std::vector<size_t> shape = { 1, 784 };
	const std::vector<float>& data = grid.getGrid();
	tbml::Tensor input(shape, data);

	// Predict the digit
	tbml::Tensor output = network.propogate(input); // 1x10 tensor

	// Get the digit
	int digit = 0;
	float max = -1.0f;
	for (int i = 0; i < 10; i++)
	{
		if (output(0, i) > max)
		{
			max = output(0, i);
			digit = i;
		}
	}

	// Set the text
	this->guessText.setString("Predicted: " + std::to_string(digit));

	// Set the chances
	for (int i = 0; i < 10; i++)
	{
		this->guessChances[i].setSize(sf::Vector2f(100.0f * output(0, i), this->guessChances[i].getSize().y));
	}
}

#pragma once
#include <NeuralNetwork.h>

class NNRenderer
{
public:
	void render(sf::RenderWindow* window, const tbml::nn::NeuralNetwork& network);

private:
	tbml::nn::NeuralNetwork* network = nullptr;
};

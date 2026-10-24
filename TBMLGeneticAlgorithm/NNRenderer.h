#pragma once
#include <NeuralNetwork.h>

struct NNInputSpec
{
	std::string label;
	float scale = 1.0f;
	float offset = 0.0f;
};

class NNRenderer
{
public:
	NNRenderer(std::vector<NNInputSpec> inputs, std::vector<std::string> outputLabels);

	void render(sf::RenderWindow* window, const tbml::nn::NeuralNetwork& network, const tbml::Tensor& rawInput) const;

private:
	std::vector<NNInputSpec> inputs;
	std::vector<std::string> outputLabels;
};

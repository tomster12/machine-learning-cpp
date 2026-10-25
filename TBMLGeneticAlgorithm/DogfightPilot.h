#pragma once

#include "CommonImpl.h"
#include "DogfightArena.h"

class DogfightPilot
{
public:
	static constexpr size_t RAY_COUNT = 8;
	static constexpr size_t NEAREST_ROCKS = 2;
	static constexpr size_t INPUT_COUNT = 32;
	static constexpr size_t OUTPUT_COUNT = 6;

	explicit DogfightPilot(tbml::nn::NeuralNetwork&& network);

	DogfightShip::Controls think(const DogfightArena& arena, size_t shipIndex);

	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	void sense(const DogfightArena& arena, size_t shipIndex);

	tbml::nn::NeuralNetwork network;
	tbml::Tensor netInput;
};

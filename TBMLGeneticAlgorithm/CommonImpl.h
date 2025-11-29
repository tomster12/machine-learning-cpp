#pragma once

#include "GenepoolSimulation.h"
#include "NeuralNetwork.h"
#include "Tensor.h"

class VectorListGenome : public tbml::ga::Genome<VectorListGenome>
{
public:
	VectorListGenome() {}
	VectorListGenome(int dataSize);
	VectorListGenome(std::vector<sf::Vector2f>&& values);

	VectorListGenome::GenomeCPtr crossover(const VectorListGenome::GenomeCPtr& otherGenome, float chance) const override;
	const std::vector<sf::Vector2f>& getValues() const;
	const sf::Vector2f getValue(int index) const;
	const size_t getSize() const;

private:
	std::vector<sf::Vector2f> values;
	int dataSize = 0;
};

class NNGenome : public tbml::ga::Genome<NNGenome>
{
public:
	NNGenome() {};
	NNGenome(tbml::nn::NeuralNetwork&& network);

	NNGenome::GenomeCPtr crossover(const NNGenome::GenomeCPtr& otherData, float mutateChance) const override;
	const tbml::nn::NeuralNetwork& getNetwork() const;
	tbml::nn::NeuralNetwork copyNetwork() const;
	size_t getInputSize() const;
	void print() const;

private:
	tbml::nn::NeuralNetwork network;
};

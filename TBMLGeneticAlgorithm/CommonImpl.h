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

	VectorListGenome::GenomePtr crossover(const VectorListGenome::GenomePtr& otherGenome, float chance) const override;
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
	NNGenome(std::vector<size_t> layerSizes);
	NNGenome(std::vector<size_t> layerSizes, std::vector<tbml::fn::ActivationFunction> actFns);
	NNGenome(tbml::nn::NeuralNetwork&& network);

	NNGenome::GenomePtr crossover(const NNGenome::GenomePtr& otherData, float mutateChance) const override;
	const tbml::Tensor& propogate(const tbml::Tensor& input);
	size_t getInputSize() const { return this->network.getInputShape()[0]; }
	void print() const;

private:
	tbml::nn::NeuralNetwork network;
};

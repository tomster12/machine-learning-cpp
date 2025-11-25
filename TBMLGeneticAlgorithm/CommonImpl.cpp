#include "stdafx.h"
#include "CommonImpl.h"
#include "Tensor.h"

VectorListGenome::VectorListGenome(int dataSize)
{
	this->dataSize = dataSize;
	this->values = std::vector<sf::Vector2f>(dataSize);
	for (int i = 0; i < this->dataSize; i++)
	{
		this->values[i].x = tbml::fn::getRandomFloat() * 2 - 1;
		this->values[i].y = tbml::fn::getRandomFloat() * 2 - 1;
	}
}

VectorListGenome::VectorListGenome(std::vector<sf::Vector2f>&& values)
{
	this->dataSize = (int)values.size();
	this->values = std::move(values);
}

VectorListGenome::GenomeCPtr VectorListGenome::crossover(const VectorListGenome::GenomeCPtr& otherData, float mutateChance) const
{
	std::vector<sf::Vector2f> newValues(this->getSize());

	for (size_t i = 0; i < this->getSize(); i++)
	{
		if (tbml::fn::getRandomFloat() < mutateChance)
		{
			newValues[i].x = tbml::fn::getRandomFloat() * 2 - 1;
			newValues[i].y = tbml::fn::getRandomFloat() * 2 - 1;
		}
		else
		{
			if (i % 2 == 0) newValues[i] = this->values[i];
			else newValues[i] = otherData->values[i];
		}
	}

	return std::make_shared<VectorListGenome>(std::move(newValues));
}

const std::vector<sf::Vector2f>& VectorListGenome::getValues() const { return values; };

const sf::Vector2f VectorListGenome::getValue(int index) const { return this->values[index]; }

const size_t VectorListGenome::getSize() const { return this->dataSize; }

NNGenome::NNGenome(tbml::nn::NeuralNetwork&& network)
	: network(std::move(network))
{}

NNGenome::GenomeCPtr NNGenome::crossover(const NNGenome::GenomeCPtr& otherData, float mutateChance) const
{
	const std::vector<tbml::nn::Layer::BasePtr>& layers = this->network.getLayers();
	const std::vector<tbml::nn::Layer::BasePtr>& otherLayers = otherData->network.getLayers();
	std::vector<tbml::nn::Layer::BasePtr> newLayers(layers.size());
	for (size_t i = 0; i < layers.size(); i++)
	{
		// Check if layers are Dense
		if (typeid(*layers[i]) != typeid(tbml::nn::Layer::Dense) || typeid(*otherLayers[i]) != typeid(tbml::nn::Layer::Dense))
		{
			newLayers[i] = layers[i]->clone();
			continue;
		}

		// Pull out dense layers
		const tbml::nn::Layer::Dense& denseLayer = dynamic_cast<const tbml::nn::Layer::Dense&>(*layers[i]);
		const tbml::nn::Layer::Dense& otherDenseLayer = dynamic_cast<const tbml::nn::Layer::Dense&>(*otherLayers[i]);

		// Perform crossover
		const tbml::Tensor& weights = denseLayer.getWeights();
		const tbml::Tensor& otherWeights = otherDenseLayer.getWeights();
		const tbml::Tensor& biases = denseLayer.getBias();
		const tbml::Tensor& otherBiases = otherDenseLayer.getBias();
		tbml::Tensor newWeights = weights.ewised(otherWeights, [&](float a, float b) -> float
		{
			if (tbml::fn::getRandomFloat() < mutateChance) return tbml::fn::getRandomFloat() * 2 - 1;
			if (tbml::fn::getRandomFloat() < 0.5f) return a;
			return b;
		});
		tbml::Tensor newBiases = biases.ewised(otherBiases, [&](float a, float b) -> float
		{
			if (tbml::fn::getRandomFloat() < mutateChance) return tbml::fn::getRandomFloat() * 2 - 1;
			if (tbml::fn::getRandomFloat() < 0.5f) return a;
			return b;
		});

		// Create new dense layer
		newLayers[i] = std::make_shared<tbml::nn::Layer::Dense>(std::move(newWeights), std::move(newBiases));
	}

	return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork(std::move(newLayers)));
}

void NNGenome::print() const { this->network.print(); }

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

namespace
{
	float getRandomGaussian()
	{
		// Box-Muller transform
		const float u = std::max(tbml::fn::getRandomFloat(), 1e-6f);
		const float v = tbml::fn::getRandomFloat();
		return std::sqrt(-2.0f * std::log(u)) * std::cos(6.28318531f * v);
	}
}

NNGenome::NNGenome(tbml::nn::NeuralNetwork&& network, float perturbStrength)
	: network(std::move(network)), perturbStrength(perturbStrength)
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
		auto inheritOrMutate = [&](float a, float b) -> float
		{
			const float inherited = tbml::fn::getRandomFloat() < 0.5f ? a : b;
			if (tbml::fn::getRandomFloat() >= mutateChance) return inherited;

			// Nudge the inherited value, or replace it outright when perturbation is off
			if (this->perturbStrength > 0.0f) return inherited + getRandomGaussian() * this->perturbStrength;
			return tbml::fn::getRandomFloat() * 2 - 1;
		};
		tbml::Tensor newWeights = weights.ewised(otherWeights, inheritOrMutate);
		tbml::Tensor newBiases = biases.ewised(otherBiases, inheritOrMutate);

		// Create new dense layer
		newLayers[i] = std::make_shared<tbml::nn::Layer::Dense>(std::move(newWeights), std::move(newBiases));
	}

	return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork(std::move(newLayers)), this->perturbStrength);
}

const tbml::nn::NeuralNetwork& NNGenome::getNetwork() const
{
	return this->network;
}

tbml::nn::NeuralNetwork NNGenome::copyNetwork() const
{
	return this->network;
}

size_t NNGenome::getInputSize() const
{
	return this->network.getInputShape()[0];
}

void NNGenome::print() const { this->network.print(); }

#include "stdafx.h"
#include "NNRenderer.h"
#include "UITheme.h"

namespace
{
	using DenseLayer = tbml::nn::Layer::Dense;

	struct Column
	{
		const DenseLayer* dense = nullptr;
		const tbml::Tensor* activation = nullptr;
	};

	sf::Color neuronColour(float value)
	{
		// Dark when idle, accent for positive activation, coral for negative
		const float t = std::min(1.0f, std::abs(std::tanh(value)));
		const sf::Color idle(38, 44, 52);
		const sf::Color target = value >= 0.0f ? theme::TEXT_PRIMARY : theme::NEGATIVE;
		return sf::Color(
			static_cast<std::uint8_t>(idle.r + (target.r - idle.r) * t),
			static_cast<std::uint8_t>(idle.g + (target.g - idle.g) * t),
			static_cast<std::uint8_t>(idle.b + (target.b - idle.b) * t));
	}

	float activationAt(const tbml::Tensor* tensor, size_t index)
	{
		if (tensor == nullptr || tensor->getSize() <= index) return 0.0f;
		return tensor->getData()[index];
	}
}

NNRenderer::NNRenderer(std::vector<NNInputSpec> inputs, std::vector<std::string> outputLabels)
	: inputs(std::move(inputs)), outputLabels(std::move(outputLabels))
{}

void NNRenderer::render(sf::RenderWindow* window, const tbml::nn::NeuralNetwork& network, const tbml::Tensor& rawInput) const
{
	// Each Dense layer is a column, with its activation being the output of the layer(s) following it
	const auto& layers = network.getLayers();
	std::vector<Column> columns;
	for (size_t i = 0; i < layers.size(); i++)
	{
		const DenseLayer* dense = dynamic_cast<const DenseLayer*>(layers[i].get());
		if (dense == nullptr) continue;

		size_t last = i;
		while (last + 1 < layers.size() && dynamic_cast<const DenseLayer*>(layers[last + 1].get()) == nullptr) last++;
		columns.push_back({ dense, layers[last]->getOutputPtr() });
	}
	if (columns.empty()) return;

	// Neuron count per column, including the input column
	std::vector<size_t> sizes;
	sizes.push_back(columns[0].dense->getWeights().getShape(0));
	for (const Column& column : columns) sizes.push_back(column.dense->getWeights().getShape(1));
	const size_t maxSize = *std::max_element(sizes.begin(), sizes.end());

	// Scale the raw network inputs so they colour sensibly
	std::vector<float> inputValues(sizes.front(), 0.0f);
	for (size_t i = 0; i < inputValues.size() && i < inputs.size() && i < rawInput.getSize(); i++)
	{
		inputValues[i] = rawInput.getData()[i] / inputs[i].scale + inputs[i].offset;
	}

	// Panel in the top right, tall enough for the widest layer
	// Tall networks squeeze together to stay inside the window
	const float chrome = theme::HEADER_INSET + theme::HEADER_HEIGHT + theme::GAP + theme::PADDING + 2.0f * theme::MARGIN;
	const float spacing = std::clamp((static_cast<float>(window->getSize().y) - chrome) / static_cast<float>(maxSize), 13.0f, 24.0f);
	const sf::Vector2f size(264.0f, theme::HEADER_INSET + theme::HEADER_HEIGHT + theme::GAP + spacing * static_cast<float>(maxSize) + theme::PADDING);
	const sf::Vector2f origin(static_cast<float>(window->getSize().x) - theme::MARGIN - size.x, theme::MARGIN);
	theme::drawPanel(window, origin, size);
	theme::drawHeader(window, "Elite network", { origin.x + theme::PADDING, origin.y + theme::HEADER_INSET }, size.x - 2.0f * theme::PADDING);

	// Layout
	const float left = origin.x + theme::PADDING + 46.0f;
	const float right = origin.x + size.x - theme::PADDING - 40.0f;
	const float top = origin.y + theme::HEADER_INSET + theme::HEADER_HEIGHT + theme::GAP;
	const float height = size.y - (top - origin.y) - theme::PADDING;
	const float radius = std::min(spacing * 0.38f, 9.0f);
	const float midY = top + height / 2.0f;
	const size_t columnCount = sizes.size();

	auto neuronPos = [&](size_t column, size_t index)
	{
		const float x = left + (right - left) * static_cast<float>(column) / static_cast<float>(std::max<size_t>(1, columnCount - 1));
		const float y = midY + (static_cast<float>(index) - (static_cast<float>(sizes[column]) - 1.0f) / 2.0f) * spacing;
		return sf::Vector2f(x, y);
	};

	// Activation of a neuron, used to light up the connections leaving it
	auto neuronValue = [&](size_t column, size_t index)
	{
		if (column == 0) return inputValues[index];
		return activationAt(columns[column - 1].activation, index);
	};

	// Connections, with strength from the weight and brightness from how active the source neuron is
	sf::VertexArray edges(sf::PrimitiveType::Lines);
	for (size_t k = 0; k < columns.size(); k++)
	{
		const tbml::Tensor& weights = columns[k].dense->getWeights();
		float maxWeight = 1e-6f;
		for (float w : weights.getData()) maxWeight = std::max(maxWeight, std::abs(w));

		for (size_t i = 0; i < sizes[k]; i++)
		{
			const float signal = 0.35f + 0.65f * std::min(1.0f, std::abs(std::tanh(neuronValue(k, i))));
			for (size_t j = 0; j < sizes[k + 1]; j++)
			{
				const float w = weights.at(i, j);
				const float strength = std::abs(w) / maxWeight;
				const sf::Color base = w >= 0.0f ? theme::TEXT_PRIMARY : theme::NEGATIVE;
				const sf::Color colour(base.r, base.g, base.b, static_cast<std::uint8_t>(20.0f + 200.0f * strength * signal));
				edges.append(sf::Vertex{ neuronPos(k, i), colour });
				edges.append(sf::Vertex{ neuronPos(k + 1, j), colour });
			}
		}
	}
	window->draw(edges);

	// Neurons
	sf::CircleShape neuron(radius);
	neuron.setOrigin({ radius, radius });
	neuron.setOutlineColor(sf::Color(255, 255, 255, 60));
	neuron.setOutlineThickness(1.0f);
	for (size_t c = 0; c < columnCount; c++)
	{
		for (size_t i = 0; i < sizes[c]; i++)
		{
			neuron.setPosition(neuronPos(c, i));
			neuron.setFillColor(neuronColour(neuronValue(c, i)));
			window->draw(neuron);
		}
	}

	// Labels
	if (inputs.size() == sizes.front())
	{
		for (size_t i = 0; i < inputs.size(); i++)
		{
			const sf::Vector2f pos = neuronPos(0, i);
			theme::drawText(window, inputs[i].label, { pos.x - radius - 6.0f, pos.y }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::RIGHT);
		}
	}
	if (outputLabels.size() == sizes.back())
	{
		for (size_t i = 0; i < outputLabels.size(); i++)
		{
			const sf::Vector2f pos = neuronPos(columnCount - 1, i);
			theme::drawText(window, outputLabels[i], { pos.x + radius + 6.0f, pos.y }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::LEFT);
		}
	}
}

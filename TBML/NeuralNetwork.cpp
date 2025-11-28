#include "stdafx.h"
#include "NeuralNetwork.h"
#include "Utility.h"
#include "omp.h"
#include "tbmlGlobal.h"

namespace tbml
{
	namespace nn
	{
		namespace Layer
		{
			BasePtr deserialize(std::istream& is)
			{
				std::string type;
				is >> type;

				if (type == "Dense")
				{
					Tensor weights = Tensor::deserialize(is);
					Tensor bias = Tensor::deserialize(is);
					return std::make_shared<Dense>(std::move(weights), std::move(bias));
				}
				else if (type == "ReLU")
				{
					return std::make_shared<ReLU>();
				}
				else if (type == "Sigmoid")
				{
					return std::make_shared<Sigmoid>();
				}
				else if (type == "TanH")
				{
					return std::make_shared<TanH>();
				}
				else if (type == "Softmax")
				{
					return std::make_shared<Softmax>();
				}

				throw std::runtime_error("Unknown layer type");
			}
		}

		namespace Layer
		{
			Dense::Dense(const Dense& other)
			{
				weights = other.weights;
				bias = other.bias;
			}

			Dense::Dense(size_t inputSize, size_t outputSize, InitType initType, bool useBias)
			{
				weights = Tensor({ inputSize, outputSize }, 0);

				if (initType == InitType::RANDOM)
				{
					weights.map([](float _) { return fn::getRandomFloat() * 2 - 1; });
				}

				if (useBias)
				{
					bias = Tensor({ 1, outputSize }, 0);

					if (initType == InitType::RANDOM)
					{
						bias.map([](float _) { return fn::getRandomFloat() * 2 - 1; });
					}
				}

				gradWeights = Tensor(weights.getShape(), 0);
				gradBias = Tensor(bias.getShape(), 0);
			}

			Dense::Dense(Tensor&& weights, Tensor&& bias)
				: weights(std::move(weights)), bias(std::move(bias))
			{
				gradWeights = Tensor(weights.getShape(), 0);
				gradBias = Tensor(bias.getShape(), 0);
			}

			void Dense::propogateMut(Tensor& input) const
			{
				assert(input.getDims() == 2 && input.getShape(1) == weights.getShape(0) && "Input shape does not match weights shape");

				// Mutably propogate input with weights and bias
				input.matmul(weights).add(bias, 0);
			}

			const Tensor* Dense::propogatePtr(const Tensor* input)
			{
				assert(input->getDims() == 2 && input->getShape(1) == weights.getShape(0) && "Input shape does not match weights shape");

				// Propogate input with weights and bias
				// Retain input and output for backprop
				this->input = input;
				input->matmul_to(weights, output).add(bias, 0);
				return &output;
			}

			void Dense::backpropogate(const Tensor* gradOutput)
			{
				assert(gradOutput->getDims() == 2 && gradOutput->getShape(1) == weights.getShape(1) && "gradOutput shape does not match weights shape");

				// Calculate pd to neuron in and layer in
				gradOutput->matmul_to(weights.transposed(), gradInput);

				int batchSize = (int)input->getShape(0);
				int rows = (int)weights.getShape(0);
				int cols = (int)weights.getShape(1);

				gradWeights.zero();
				gradBias.zero();

				// Calculate pd to weights and bias as average of batches
				int threads = tbml::getOmpThreads();
				#pragma omp parallel for num_threads(threads)
				for (int batchRow = 0; batchRow < batchSize; batchRow++)
				{
					for (int i = 0; i < rows; i++)
					{
						const float batchInputI = input->at(batchRow, i);
						for (int j = 0; j < cols; j++)
						{
							const float batchOutputJ = gradOutput->at(batchRow, j);
							gradWeights(i, j) += (batchInputI * batchOutputJ) / batchSize;
							if (i == 0)
							{
								gradBias(0, j) += batchOutputJ / batchSize;
							}
						}
					}
				}
			}

			void Dense::gradientDescent(float learningRate, float momentumRate)
			{
				// Apply gradient descent with momentum
				momentumWeights = (momentumWeights * momentumRate) - (gradWeights * learningRate);
				momentumBias = (momentumBias * momentumRate) - (gradBias * learningRate);
				weights += momentumWeights;
				bias += momentumBias;
			}

			void Dense::print() const
			{
				weights.print("Weights:");
				bias.print("Bias:");
			}

			BasePtr Dense::clone() const
			{
				return std::make_shared<Dense>(*this);
			}

			void Dense::serialize(std::ostream& os) const
			{
				os << "Dense\n";
				weights.serialize(os);
				bias.serialize(os);
			}
		}

		namespace Layer
		{
			void ReLU::propogateMut(Tensor& input) const
			{
				// Mutably propogate input with ReLU activation
				input.map([](float x) { return std::max(0.0f, x); });
			}

			const Tensor* ReLU::propogatePtr(const Tensor* input)
			{
				// Propogate input with ReLU activation
				// Retain input and output for backprop
				this->input = input;
				output = input->mapped([](float x) { return std::max(0.0f, x); });
				return &output;
			}

			void ReLU::backpropogate(const Tensor* gradOutput)
			{
				// Calculate grad output to input * grad output
				gradInput = input->mapped([](float x) { return x > 0 ? 1.0f : 0.0f; }) * (*gradOutput);
			}

			BasePtr ReLU::clone() const
			{
				return std::make_shared<ReLU>();
			}

			void ReLU::serialize(std::ostream& os) const
			{
				os << "ReLU\n";
			}
		}

		namespace Layer
		{
			void Sigmoid::propogateMut(Tensor& input) const
			{
				// Mutably propogate input with Sigmoid activation
				input.map([this](float x) { return sigmoid(x); });
			}

			const Tensor* Sigmoid::propogatePtr(const Tensor* input)
			{
				// Propogate input with Sigmoid activation
				// Retain input and output for backprop
				this->input = input;
				output = input->mapped([this](float x) { return sigmoid(x); });
				return &output;
			}

			void Sigmoid::backpropogate(const Tensor* gradOutput)
			{
				// Calculate grad output to input * grad output
				gradInput = input->mapped([this](float x)
				{
					float sv = sigmoid(x);
					return sv * (1.0f - sv);
				}) * (*gradOutput);
			}

			BasePtr Sigmoid::clone() const
			{
				return std::make_shared<Sigmoid>();
			}

			void Sigmoid::serialize(std::ostream& os) const
			{
				os << "Sigmoid\n";
			}
		}

		namespace Layer
		{
			void TanH::propogateMut(Tensor& input) const
			{
				// Mutably propogate input with TanH activation
				input.map([](float x) { return tanhf(x); });
			}

			const Tensor* TanH::propogatePtr(const Tensor* input)
			{
				// Propogate input with TanH activation
				// Retain input and output for backprop
				this->input = input;
				output = input->mapped([](float x) { return tanhf(x); });
				return &output;
			}

			void TanH::backpropogate(const Tensor* gradOutput)
			{
				// Calculate grad output to input * grad output
				gradInput = input->mapped([](float x)
				{
					float th = tanhf(x);
					return 1.0f - (th * th);
				}) * (*gradOutput);
			}

			BasePtr TanH::clone() const
			{
				return std::make_shared<TanH>();
			}

			void TanH::serialize(std::ostream& os) const
			{
				os << "TanH\n";
			}
		}

		namespace Layer
		{
			void Softmax::propogateMut(Tensor& input) const
			{
				auto shape = input.getShape();
				assert(shape.size() == 2);

				// Independent per batch
				for (size_t row = 0; row < shape[0]; row++)
				{
					// Calculate max of batch for stability
					float max = input(row, 0);
					for (size_t i = 1; i < shape[1]; i++) max = std::max(max, input(row, i));

					// SoftMax of each element in batch = e^(X(i) - max) / Σ e^(X(i) - max)
					float sum = 0.0;
					for (size_t i = 0; i < shape[1]; i++)
					{
						input(row, i) = std::exp(input(row, i) - max);
						sum += input(row, i);
					}
					for (size_t i = 0; i < shape[1]; i++)
					{
						input(row, i) /= sum;
					}
				}
			}

			const Tensor* Softmax::propogatePtr(const Tensor* input)
			{
				auto shape = input->getShape();
				assert(shape.size() == 2);

				// Propogate input with SoftMax activation
				// Retain input and output for backprop
				this->input = input;

				// Independent per batch
				output = Tensor(shape, 0);
				for (size_t row = 0; row < shape[0]; row++)
				{
					// Calculate max of batch for stability
					float max = input->at(row, 0);
					for (size_t i = 1; i < shape[1]; i++) max = std::max(max, input->at(row, i));

					// SoftMax of each element in batch = e^(X(i) - max) / Σ e^(X(i) - max)
					float sum = 0.0;
					for (size_t i = 0; i < shape[1]; i++)
					{
						output(row, i) = std::exp(input->at(row, i) - max);
						sum += output.at(row, i);
					}
					for (size_t i = 0; i < shape[1]; i++)
					{
						output(row, i) /= sum;
					}
				}
				return &output;
			}

			void Softmax::backpropogate(const Tensor* gradOutput)
			{
				auto shape = output.getShape();
				assert(shape.size() == 2);

				// Calculate grad output to input * grad output
				gradInput = Tensor(input->getShape(), 0);

				// Independent per row
				for (size_t row = 0; row < shape[0]; row++)
				{
					// For each neuron i
					for (size_t i = 0; i < shape[1]; i++)
					{
						float Zi = output(row, i);
						float sum = 0.0;
						for (size_t j = 0; j < shape[1]; j++)
						{
							float Zj = output(row, j);
							int kronekerDelta = (i == j) ? 1 : 0;
							float dSij = (Zj * (kronekerDelta - Zi));
							gradInput(row, i) += dSij * gradOutput->at(row, j);
						}
					}
				}
			}

			BasePtr Softmax::clone() const
			{
				return std::make_shared<Softmax>();
			}

			void Softmax::serialize(std::ostream& os) const
			{
				os << "Softmax\n";
			}
		}

		TensorBatcher::TensorBatcher(const Tensor& input, const Tensor& expected, int batchSize, bool shuffle, bool preload)
			: input(input), expected(expected)
		{
			assert(input.getShape(0) == expected.getShape(0) && "Input and expected shape mismatch");

			// Setup batch size and count
			this->batchSize = batchSize == -1 ? input.getShape(0) : batchSize;
			batchCount = (int)std::ceil(input.getShape(0) / (float)this->batchSize);

			// Setup indices
			indices.resize(input.getShape(0));
			std::iota(indices.begin(), indices.end(), 0);
			if (shuffle)
			{
				static thread_local std::mt19937 rng{ std::random_device{}() };
				std::shuffle(indices.begin(), indices.end(), rng);
			}
			if (preload) loadBatches();
		}

		void TensorBatcher::shuffleAndLoad()
		{
			static thread_local std::mt19937 rng{ std::random_device{}() };
			std::shuffle(indices.begin(), indices.end(), rng);
			loadBatches();
		}

		void TensorBatcher::loadBatches()
		{
			// Setup all batches
			inputBatches.clear();
			expectedBatches.clear();
			for (size_t i = 0; i < batchCount; i++)
			{
				size_t start = i * this->batchSize;
				size_t end = std::min(start + this->batchSize, input.getShape(0));
				std::vector<size_t> batchIndices(indices.begin() + start, indices.begin() + end);
				inputBatches.push_back(input.sample(0, batchIndices));
				expectedBatches.push_back(expected.sample(0, batchIndices));
			}
		}

		void NeuralNetwork::addLayer(Layer::BasePtr&& layer)
		{
			layers.push_back(std::move(layer));
		}

		Tensor NeuralNetwork::propogate(const Tensor& input) const
		{
			if (layers.size() == 0) return Tensor(Tensor::ZERO);

			// Copy to local, propogate layers mutably
			Tensor current = input;
			for (size_t i = 0; i < layers.size(); i++) layers[i]->propogateMut(current);
			return current;
		}

		void NeuralNetwork::propogateMut(Tensor& input) const
		{
			if (layers.size() == 0) return;

			// Directly propogate layers with mutable input
			for (size_t i = 0; i < layers.size(); i++) layers[i]->propogateMut(input);
		}

		const Tensor* NeuralNetwork::propogatePtr(const Tensor* input)
		{
			if (layers.size() == 0) return nullptr;

			// Propogate layers with referencable tensor
			// Used to track values for backpropogation
			layers[0]->propogatePtr(input);
			for (size_t i = 1; i < layers.size(); i++) layers[i]->propogatePtr(layers[i - 1]->getOutputPtr());
			return layers[layers.size() - 1]->getOutputPtr();
		}

		void NeuralNetwork::train(const Tensor& input, const Tensor& expected, const tbml::fn::LossFunctionPtr lossFn, const TrainingConfig& config)
		{
			// Setup data batchers
			TensorBatcher batcher(input, expected, config.batchSize, false, false);
			size_t maxBatch = batcher.getBatchCount();

			// Train for each batch for each epoch
			size_t maxEpoch = config.maxEpoch == -1 ? MAX_EPOCHS : config.maxEpoch;

			if (config.logLevel > 0)
			{
				printf("Training started:\n");
				printf("\tMax Epochs: %zu\n", maxEpoch);
				printf("\tBatch Size: %d\n", config.batchSize);
				printf("\tLearning Rate: %f\n", config.learningRate);
				printf("\tMomentum Rate: %f\n", config.momentumRate);
				printf("\tError Threshold: %f\n\n", config.errorThreshold);
			}

			std::chrono::steady_clock::time_point tTrainStart = std::chrono::steady_clock::now();
			std::chrono::steady_clock::time_point tEpochStart = tTrainStart;
			std::chrono::steady_clock::time_point tBatchStart = tTrainStart;

			size_t epoch = 0;
			for (; epoch < maxEpoch; epoch++)
			{
				batcher.shuffleAndLoad();
				float epochLoss = 0.0f;
				for (size_t batch = 0; batch < maxBatch; batch++)
				{
					// Get input and expected batch
					const Tensor& inputBatch = batcher.getBatchInput(batch);
					const Tensor& expectedBatch = batcher.getBatchExpected(batch);

					// Propogate input then calculate loss
					const Tensor* predicted = propogatePtr(&inputBatch);
					float batchLoss = lossFn->calculate(*predicted, expectedBatch);
					epochLoss += batchLoss / maxBatch;

					// Backpropogate loss through each layer
					const Tensor gradLossToOut = lossFn->derivative(*predicted, expectedBatch);
					layers[layers.size() - 1]->backpropogate(&gradLossToOut);
					for (int i = (int)layers.size() - 2; i >= 0; i--)
					{
						layers[i]->backpropogate(layers[i + 1]->getGradInputPtr());
					}

					// Apply gradient descent
					for (size_t j = 0; j < layers.size(); j++)
					{
						layers[j]->gradientDescent(config.learningRate, config.momentumRate);
					}

					if (config.logLevel >= 3)
					{
						if ((batch + 1) % config.logFrequency == 0)
						{
							std::chrono::steady_clock::time_point tBatchEnd = std::chrono::steady_clock::now();
							auto us = std::chrono::duration_cast<std::chrono::microseconds>(tBatchEnd - tBatchStart);
							printf("Epoch [%zd / %zd], Batch [%zd / %zd]: Loss: %.3f, Time: %.3fms\n", epoch + 1, maxEpoch, batch + 1, maxBatch, batchLoss, us.count() / 1000.0f);
							tBatchStart = tBatchEnd;
						}
					}
				}

				if (config.logLevel >= 2)
				{
					std::chrono::steady_clock::time_point tEpochEnd = std::chrono::steady_clock::now();
					auto us = std::chrono::duration_cast<std::chrono::microseconds>(tEpochEnd - tEpochStart);
					printf("Epoch [%zd / %zd]: Average Loss: %.3f, Total Time: %.3fms\n", epoch + 1, maxEpoch, epochLoss, us.count() / 1000.0f);
					tEpochStart = tEpochEnd;
					tBatchStart = tEpochEnd;
				}

				// Exit if error threshold is met
				if (epochLoss < config.errorThreshold) break;
			}

			if (config.logLevel >= 1)
			{
				std::chrono::steady_clock::time_point tTrainEnd = std::chrono::steady_clock::now();
				auto us = std::chrono::duration_cast<std::chrono::microseconds>(tTrainEnd - tTrainStart);
				printf("Training complete for %zd epochs, Time taken: %.3fms\n\n", epoch, us.count() / 1000.0f);
			}
		}

		size_t NeuralNetwork::getParameterCount() const
		{
			size_t count = 0;
			for (const auto& layer : layers) count += layer->getParameterCount();
			return count;
		}

		void NeuralNetwork::print() const
		{
			for (const auto& layer : layers) layer->print();
		}

		void NeuralNetwork::saveToFile(const std::string& filename) const
		{
			std::ofstream file(filename, std::ios::binary);
			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open file for writing");
			}

			// Write number of layers
			file << layers.size() << "\n";

			// Write each layer
			for (const auto& layer : layers)
			{
				layer->serialize(file);
			}
		}

		NeuralNetwork loadFromFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::binary);
			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open file for reading");
			}

			// Read the number of layers
			size_t layerCount;
			file >> layerCount;

			// Read each layer
			std::vector<Layer::BasePtr> layers;
			for (size_t i = 0; i < layerCount; i++)
			{
				layers.push_back(Layer::deserialize(file));
			}

			return NeuralNetwork(std::move(layers));
		}
	}
}

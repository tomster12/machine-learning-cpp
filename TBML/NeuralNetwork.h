#pragma once

#include "Utility.h"
#include "Tensor.h"

namespace tbml
{
	namespace nn
	{
		namespace Layer
		{
			class Base
			{
			public:
				Base() = default;
				virtual ~Base() = default;
				Base(const Base&) = delete;
				Base& operator=(const Base&) = delete;
				Base(Base&&) = delete;
				Base& operator=(Base&&) = delete;

				virtual void propogateMut(Tensor& input) const = 0;
				virtual const Tensor* propogatePtr(const Tensor* input) = 0;
				virtual void backpropogate(const Tensor* gradOutput) = 0;
				virtual void gradientDescent(float learningRate, float momentumRate) {};
				virtual std::shared_ptr<Base> clone() const = 0;
				virtual void print() const {}
				virtual void serialize(std::ostream& os) const = 0;
				virtual std::vector<size_t> getInputShape() const = 0;
				virtual std::vector<size_t> getOutputShape() const = 0;
				virtual size_t getParameterCount() const { return 0; };
				const Tensor* getOutputPtr() const { return &output; };
				const Tensor* getGradInputPtr() const { return &gradInput; };

			protected:
				Tensor output;
				Tensor gradInput;
				const Tensor* input = nullptr;
			};

			using BasePtr = std::shared_ptr<Base>;

			class Dense : public Base
			{
			public:
				enum class InitType { ZERO, RANDOM };

				Dense(const Dense& other);
				Dense(size_t inputSize, size_t outputSize, InitType initType = InitType::RANDOM, bool useBias = true);
				Dense(Tensor&& weights, Tensor&& bias);

				void initGradTensors();
				virtual void propogateMut(Tensor& input) const override;
				virtual const Tensor* propogatePtr(const Tensor* input) override;
				void backpropogate(const Tensor* gradOutput) override;
				void gradientDescent(float learningRate, float momentumRate) override;
				virtual void print() const override;
				virtual BasePtr clone() const override;
				std::vector<size_t> getInputShape() const override { return { weights.getShape(0) }; }
				std::vector<size_t> getOutputShape() const override { return { weights.getShape(1) }; }
				size_t getParameterCount() const override { return weights.getSize() + bias.getSize(); }
				const Tensor& getWeights() const { return weights; }
				const Tensor& getBias() const { return bias; }
				virtual void serialize(std::ostream& os) const override;

			private:
				Tensor weights;
				Tensor bias;
				Tensor gradWeights;
				Tensor gradBias;
				Tensor momentumWeights;
				Tensor momentumBias;
				std::vector<std::vector<float>> threadGradWeights;
				std::vector<std::vector<float>> threadGradBias;
			};

			class ReLU : public Base
			{
			public:
				virtual void propogateMut(Tensor& input) const override;
				virtual const Tensor* propogatePtr(const Tensor* input) override;
				void backpropogate(const Tensor* gradOutput) override;
				virtual BasePtr clone() const override;
				std::vector<size_t> getInputShape() const override { return { 1 }; }
				std::vector<size_t> getOutputShape() const override { return { 1 }; }
				virtual void serialize(std::ostream& os) const override;
			};

			class Sigmoid : public Base
			{
			public:
				virtual void propogateMut(Tensor& input) const override;
				virtual const Tensor* propogatePtr(const Tensor* input) override;
				void backpropogate(const Tensor* gradOutput) override;
				virtual BasePtr clone() const override;
				std::vector<size_t> getInputShape() const override { return { 1 }; }
				std::vector<size_t> getOutputShape() const override { return { 1 }; }
				virtual void serialize(std::ostream& os) const override;

			private:
				float sigmoid(float x) const { return 1.0f / (1.0f + std::exp(-x)); }
			};

			class TanH : public Base
			{
			public:
				virtual void propogateMut(Tensor& input) const override;
				virtual const Tensor* propogatePtr(const Tensor* input) override;
				void backpropogate(const Tensor* gradOutput) override;
				virtual BasePtr clone() const override;
				std::vector<size_t> getInputShape() const override { return { 1 }; }
				std::vector<size_t> getOutputShape() const override { return { 1 }; }
				virtual void serialize(std::ostream& os) const override;
			};

			class Softmax : public Base
			{
			public:
				virtual void propogateMut(Tensor& input) const override;
				virtual const Tensor* propogatePtr(const Tensor* input) override;
				void backpropogate(const Tensor* gradOutput) override;
				virtual BasePtr clone() const override;
				std::vector<size_t> getInputShape() const override { return { 1 }; }
				std::vector<size_t> getOutputShape() const override { return { 1 }; }
				virtual void serialize(std::ostream& os) const override;
			};

			/*
			class ConvLayer : public Layer
			{
			public:
				ConvLayer(std::vector<size_t> kernel, std::vector<size_t> stride, fn::ActivationFunction&& activationFn);
				virtual const Tensor& propogate(const Tensor& input) override;
				virtual Tensor propogate(const Tensor& input) const override;
				virtual void propogate(Tensor& input) const override;
				virtual void backpropogate(const Tensor& gradOutput) override;
				virtual void gradientDescent(float learningRate, float momentumRate) override;
				virtual LayerPtr clone() const override;

			private:
				fn::ActivationFunction activationFn;
			};

			class FlattenLayer : public Layer
			{
			public:
				FlattenLayer();
				virtual const Tensor& propogate(const Tensor& input) override;
				virtual Tensor propogate(const Tensor& input) const override;
				virtual void propogate(Tensor& input) const override;
				virtual void backpropogate(const Tensor& gradOutput) override;
				virtual LayerPtr clone() const override;
			};

			class MaxPoolLayer : public Layer
			{
			public:
				MaxPoolLayer();
				virtual const Tensor& propogate(const Tensor& input) override;
				virtual Tensor propogate(const Tensor& input) const override;
				virtual void propogate(Tensor& input) const override;
				virtual void backpropogate(const Tensor& gradOutput) override;
				virtual LayerPtr clone() const override;
			};
			*/

			static BasePtr deserialize(std::istream& is);
		}

		struct TrainingConfig
		{
			int maxEpoch = 20;
			int batchSize = -1;
			float learningRate = 0.1f;
			float momentumRate = 0.1f;
			float errorThreshold = 0.0f;
			size_t logLevel = 0;
			size_t logFrequency = 1;
		};

		class TensorBatcher
		{
		public:
			TensorBatcher(const Tensor& input, const Tensor& expected, int batchSize, bool shuffle, bool preload);
			void shuffleAndLoad();
			void loadBatches();
			const Tensor& getBatchInput(size_t batchIndex) const { return inputBatches[batchIndex]; }
			const Tensor& getBatchExpected(size_t batchIndex) const { return expectedBatches[batchIndex]; }
			size_t getBatchCount() const { return batchCount; }

		private:
			const Tensor& input;
			const Tensor& expected;
			size_t batchSize;
			size_t batchCount;
			std::vector<int> indices;
			std::vector<Tensor> inputBatches;
			std::vector<Tensor> expectedBatches;
		};

		class NeuralNetwork
		{
		public:
			static const int MAX_EPOCHS = 1'000;

			NeuralNetwork() {}
			NeuralNetwork(std::vector<Layer::BasePtr>&& layers) : layers(std::move(layers)) {}

			void addLayer(Layer::BasePtr&& layer);
			virtual Tensor propogate(const Tensor& input) const;
			virtual void propogateMut(Tensor& input) const;
			virtual const Tensor* propogatePtr(const Tensor* input);
			void train(const Tensor& input, const Tensor& expected, const tbml::fn::LossFunctionPtr lossFn, const TrainingConfig& config);
			void print() const;
			void saveToFile(const std::string& filename) const;
			std::vector<size_t> getInputShape() const { return layers[0]->getInputShape(); }
			std::vector<size_t> getOutputShape() const { return layers[layers.size() - 1]->getOutputShape(); }
			const std::vector<Layer::BasePtr>& getLayers() const { return layers; }
			size_t getParameterCount() const;

		private:
			std::vector<Layer::BasePtr> layers;
		};

		NeuralNetwork loadFromFile(const std::string& filename);
	}
};

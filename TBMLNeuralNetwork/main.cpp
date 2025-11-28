#include <vector>
#include <iostream>
#include <chrono>
#include <string>
#include <omp.h>

#include "MNIST.h"
#include "ThreadPool.h"
#include "NeuralNetwork.h"
#include "Utility.h"
#include "Tensor.h"
#include "TbmlGlobal.h"

void testTime();
void testBatch();
void testTraining();
void testSerialization();
void testMNIST();
void testMNISTSerialization();

int main()
{
	tbml::setOmpThreads(12);

	srand(0);

	testMNIST();
}

void testTime()
{
	// Create networks
	tbml::nn::NeuralNetwork network({
		std::make_shared<tbml::nn::Layer::Dense>(8, 8),
		std::make_shared<tbml::nn::Layer::Sigmoid>(),
		std::make_shared<tbml::nn::Layer::Dense>(8, 8),
		std::make_shared<tbml::nn::Layer::Sigmoid>(),
		std::make_shared<tbml::nn::Layer::Dense>(8, 8),
		std::make_shared<tbml::nn::Layer::Sigmoid>() });

	// Setup and print input
	tbml::Tensor input = tbml::Tensor({ { 1, 0, -1, 0.2f, 0.7f, -0.3f, -1, -1 } });
	input.print("Input: ");

	// Time different propogation methods
	size_t epoch = 5'000'000;
	std::chrono::steady_clock::time_point t00 = std::chrono::steady_clock::now();
	for (size_t i = 0; i < epoch; i++) network.propogate(input);
	std::chrono::steady_clock::time_point t01 = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point t10 = std::chrono::steady_clock::now();
	for (size_t i = 0; i < epoch; i++) network.propogateMut(input);
	std::chrono::steady_clock::time_point t11 = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point t20 = std::chrono::steady_clock::now();
	for (size_t i = 0; i < epoch; i++) network.propogatePtr(&input);
	std::chrono::steady_clock::time_point t21 = std::chrono::steady_clock::now();

	// Print output
	std::cout << std::endl << "Epochs: " << epoch << std::endl;
	float t0 = std::chrono::duration_cast<std::chrono::milliseconds>(t01 - t00).count() / (float)epoch;
	float t1 = std::chrono::duration_cast<std::chrono::milliseconds>(t11 - t10).count() / (float)epoch;
	float t2 = std::chrono::duration_cast<std::chrono::milliseconds>(t21 - t20).count() / (float)epoch;
	std::cout << "Prop: " << t0 << "ms" << std::endl;
	std::cout << "Prop Mut: " << t1 << "ms" << std::endl;
	std::cout << "Prop Ref: " << t2 << "ms" << std::endl;
}

void testBatch()
{
	// Create example data and batcher
	tbml::Tensor input{ std::vector<std::vector<float>>{ { 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 0 } } };
	tbml::Tensor expected{ std::vector<std::vector<float>>{ { 1 }, { 1 }, { 0 }, { 0 } } };
	tbml::nn::TensorBatcher batcher(input, expected, 3, true, true);

	// Print out batches
	size_t batchCount = batcher.getBatchCount();
	std::cout << "Batch Count: " << batchCount << std::endl;
	for (size_t i = 0; i < batchCount; i++)
	{
		batcher.getBatchInput(i).print("Batch " + std::to_string(i) + " Input: ");
		batcher.getBatchExpected(i).print("Batch " + std::to_string(i) + " Expected: ");
	}
}

void testTraining()
{
	// Create network and input
	const float L = -1.0f, H = 1.0f;
	tbml::Tensor input{ std::vector<std::vector<float>>{ { L, L }, { L, H }, { H, L }, { H, H } } };
	tbml::Tensor expected{ std::vector<std::vector<float>>{ { L }, { H }, { H }, { L } } };

	tbml::nn::NeuralNetwork network({
		std::make_shared<tbml::nn::Layer::Dense>(2, 3),
		std::make_shared<tbml::nn::Layer::TanH>(),
		std::make_shared<tbml::nn::Layer::Dense>(3, 1),
		std::make_shared<tbml::nn::Layer::TanH>() });

	// Print values and train
	input.print("Input:");
	expected.print("Expected:");
	network.propogate(input).print("Net Initial: ");
	network.train(input, expected, std::make_shared<tbml::fn::SquareError>(), { -1, -1, 0.2f, 0.85f, 0.01f, 2, 1 });
	network.propogate(input).print("Net Trained: ");
}

void testSerialization()
{
	tbml::nn::NeuralNetwork network({
		std::make_shared<tbml::nn::Layer::Dense>(2, 2),
		std::make_shared<tbml::nn::Layer::ReLU>(),
		std::make_shared<tbml::nn::Layer::Dense>(2, 1),
		std::make_shared<tbml::nn::Layer::Sigmoid>() });

	network.print();

	network.saveToFile("test.nn");
	tbml::nn::NeuralNetwork network2 = tbml::nn::loadFromFile("test.nn");

	network2.print();
}

void testMNIST()
{
	// Read training / test datasets
	size_t trainImageCount, trainImageSize, trainLabelCount;
	size_t testImageCount, testImageSize, testLabelCount;
	tbml::Tensor trainInput = MNIST::readImagesTensor("MNIST/train-images.idx3-ubyte", trainImageCount, trainImageSize);
	tbml::Tensor trainExpected = MNIST::readLabelsTensor("MNIST/train-labels.idx1-ubyte", trainLabelCount);
	tbml::Tensor testInput = MNIST::readImagesTensor("MNIST/t10k-images.idx3-ubyte", testImageCount, testImageSize);
	tbml::Tensor testExpected = MNIST::readLabelsTensor("MNIST/t10k-labels.idx1-ubyte", testLabelCount);
	assert(trainImageSize == 784 && testImageSize == 784);

	// Print out dataset information
	std::cout << "Training Image Count: " << trainImageCount << std::endl;
	trainInput.print("Training Input: ");
	trainExpected.print("Training Expected: ");
	std::cout << "\nTest Image Count: " << testImageCount << std::endl;
	testInput.print("Test Input: ");
	testExpected.print("Test Expected: ");

	// Create network and train
	// Timing (omp threads: 12, batch size: 100 => batch: ~360ms, epoch: ~2300ms) Fitness (10 epochs = 94.72%)
	tbml::nn::NeuralNetwork network({
		std::make_unique<tbml::nn::Layer::Dense>(784, 100),
		std::make_unique<tbml::nn::Layer::ReLU>(),
		std::make_unique<tbml::nn::Layer::Dense>(100, 10),
		std::make_unique<tbml::nn::Layer::Softmax>() });
	std::cout << "\nParameters: " << network.getParameterCount() << std::endl << std::endl;
	network.train(trainInput, trainExpected, std::make_shared<tbml::fn::CrossEntropy>(), { 10, 100, 0.02f, 0.9f, 0.01f, 3, 100 });

	// Test network against test data
	tbml::Tensor testPredicted = network.propogate(testInput);
	float accuracy = tbml::fn::classificationAccuracy(testPredicted, testExpected);
	std::cout << "t10k Accuracy = " << (accuracy * 100) << "%" << std::endl;

	// Save network to file
	// network.saveToFile("MNIST.nn");
}

void testMNISTSerialization()
{
	// Read training / test datasets
	size_t trainImageCount, trainImageSize, trainLabelCount;
	size_t testImageCount, testImageSize, testLabelCount;
	tbml::Tensor trainInput = MNIST::readImagesTensor("MNIST/train-images.idx3-ubyte", trainImageCount, trainImageSize);
	tbml::Tensor trainExpected = MNIST::readLabelsTensor("MNIST/train-labels.idx1-ubyte", trainLabelCount);
	tbml::Tensor testInput = MNIST::readImagesTensor("MNIST/t10k-images.idx3-ubyte", testImageCount, testImageSize);
	tbml::Tensor testExpected = MNIST::readLabelsTensor("MNIST/t10k-labels.idx1-ubyte", testLabelCount);
	assert(trainImageSize == 784 && testImageSize == 784);

	// Read network from file
	tbml::nn::NeuralNetwork network = tbml::nn::loadFromFile("MNIST.nn");

	// Print network information
	network.print();
	std::cout << "Parameters: " << network.getParameterCount() << std::endl;

	// Test network against test data
	tbml::Tensor testPredicted = network.propogate(testInput);
	float accuracy = tbml::fn::classificationAccuracy(testPredicted, testExpected);
	std::cout << "t10k Accuracy = " << (accuracy * 100) << "%" << std::endl;
}

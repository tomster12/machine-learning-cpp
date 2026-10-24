#pragma once

#include "CommonImpl.h"
#include "Body.h"

class NNDriverGenepool;
class NNDriverAgent : public tbml::ga::Agent<NNGenome>
{
public:
	NNDriverAgent(
		NNDriverAgent::GenomeCPtr&& genome, const NNDriverGenepool* genepool,
		sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, float eyeSpread, int iterationsPerTarget);

	void initVisual();
	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window);
	void calculateFitness();

	int getFinishType() const { return finishType; }
	size_t getCurrentTarget() const { return currentTarget; }
	size_t getCurrentIteration() const { return currentIteration; }
	sf::Vector2f getPosition() const { return mainBody.pos; }
	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	sf::Color stateColour(std::uint8_t alpha) const;

	const NNDriverGenepool* genepool = nullptr;
	tbml::nn::NeuralNetwork network;
	bool isVisualInit = false;
	Body mainBody;
	sf::RectangleShape mainShape;
	sf::RectangleShape eyeShape;
	sf::Color eyeColourHit;
	sf::Color eyeColourMiss;

	float maxDrivingSpeed = 2.0f;
	float steeringSpeed = 0.1f;
	float drivingAcc = 0.1f;
	float moveDrag = 0.999f;
	float eyeLength = 100.0f;
	float eyeSpread = 0.1f;
	int iterationsPerTarget = 0;

	size_t currentIteration = 0;
	size_t currentTarget = 0;
	size_t maxIterations = 0;
	size_t reachedTargetIteration = 0;
	tbml::Tensor netInput;
	float eyeHits[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float drivingSpeed = 0.0f;
	int finishType = -1;
};

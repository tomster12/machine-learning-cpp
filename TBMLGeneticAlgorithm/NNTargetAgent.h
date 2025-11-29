#pragma once

#include "CommonImpl.h"

class NNTargetGenepool;
class NNTargetAgent : public tbml::ga::Agent<NNGenome>
{
public:
	NNTargetAgent(
		NNTargetAgent::GenomeCPtr&& genome, const NNTargetGenepool* genepool,
		sf::Vector2f startPos, float radius, float moveAcc, float moveDrag, int maxIterations);

	void initVisual();
	bool evaluate() override;
	void render(sf::RenderWindow* window);
	float calculateDist();
	float calculateFitness();

private:
	const NNTargetGenepool* genepool = nullptr;
	tbml::nn::NeuralNetwork network;
	bool isVisualInit = false;
	sf::CircleShape shape;
	sf::Vector2f startPos;
	float radius = 0;
	float moveAcc = 0;
	float moveDrag = 0;
	int maxIterations = 0;
	tbml::Tensor netInput;
	sf::Vector2f pos;
	sf::Vector2f vel;
	int currentIteration = 0;
	int currentTarget = 0;
	float anger = 0.0f;
};

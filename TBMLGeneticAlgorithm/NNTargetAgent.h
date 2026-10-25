#pragma once

#include "CommonImpl.h"

class NNTargetAgent : public tbml::ga::Agent<NNGenome>
{
public:
	using Targets = std::vector<sf::Vector2f>;

	NNTargetAgent(
		NNTargetAgent::GenomeCPtr&& genome, std::shared_ptr<const Targets> targets, float targetRadius,
		sf::Vector2f startPos, float radius, float moveAcc, float moveDrag, int maxIterations);

	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window) const;

	sf::Vector2f getPosition() const { return pos; }
	sf::Vector2f getVelocity() const { return vel; }
	sf::Vector2f getAcceleration() const { return acc; }
	sf::Vector2f getTarget(size_t index) const { return (*targets)[index % targets->size()]; }
	size_t getTargetCount() const { return targets->size(); }
	float getTargetRadius() const { return targetRadius; }
	float getRadius() const { return radius; }
	size_t getTargetsHit() const { return currentTarget; }
	size_t getCurrentIteration() const { return currentIteration; }
	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	float calculateDist() const;
	void calculateFitness();

	std::shared_ptr<const Targets> targets;
	tbml::nn::NeuralNetwork network;
	float targetRadius = 0.0f;
	sf::Vector2f startPos;
	float radius = 0.0f;
	float moveAcc = 0.0f;
	float moveDrag = 0.0f;
	size_t maxIterations = 0;
	tbml::Tensor netInput;
	sf::Vector2f pos;
	sf::Vector2f vel;
	sf::Vector2f acc;
	size_t currentIteration = 0;
	size_t currentTarget = 0;
};

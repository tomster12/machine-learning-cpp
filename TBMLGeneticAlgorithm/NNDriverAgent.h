#pragma once

#include "CommonImpl.h"
#include "Body.h"
#include "DriverCourse.h"

class NNDriverAgent : public tbml::ga::Agent<NNGenome>
{
public:
	static constexpr size_t EYE_COUNT = 5;
	static constexpr size_t INPUT_COUNT = EYE_COUNT + 3;

	NNDriverAgent(
		NNDriverAgent::GenomeCPtr&& genome, std::shared_ptr<const DriverCourse> course,
		float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, float eyeSpread, int iterationsPerTarget);

	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window) const;

	int getFinishType() const { return finishType; }
	const std::shared_ptr<const DriverCourse>& getCourse() const { return course; }
	size_t getCurrentTarget() const { return currentTarget; }
	size_t getCurrentIteration() const { return currentIteration; }
	sf::Vector2f getPosition() const { return mainBody.pos; }
	sf::Vector2f getHeading() const { return { std::cos(mainBody.rot), std::sin(mainBody.rot) }; }
	float getSpeed() const { return drivingSpeed; }
	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	sf::Vector2f toWorld(sf::Vector2f local) const;
	sf::Color stateColour(std::uint8_t alpha) const;
	void calculateFitness();

	std::shared_ptr<const DriverCourse> course;
	tbml::nn::NeuralNetwork network;
	Body mainBody;

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
	float eyeHits[EYE_COUNT] = {};
	float drivingSpeed = 0.0f;
	float steer = 0.0f;
	float throttle = 0.0f;
	int finishType = -1;
};

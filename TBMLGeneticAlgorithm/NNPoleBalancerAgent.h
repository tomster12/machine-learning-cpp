#pragma once

#include "CommonImpl.h"

enum class PoleOutcome { BALANCING, FELL, OFF_TRACK, SURVIVED };

// https://researchbank.swinburne.edu.au/file/62a8df69-4a2c-407f-8040-5ac533fc2787/1/PDF%20(12%20pages).pdf
class NNPoleBalancerAgent : public tbml::ga::Agent<NNGenome>
{
public:
	static constexpr float TIME_STEP = 0.02f;
	static constexpr float METRE_TO_UNIT = 200.0f;
	static constexpr float ORIGIN_X = 700.0f;
	static constexpr float ORIGIN_Y = 700.0f;
	static constexpr float CART_WIDTH = 60.0f;
	static constexpr float CART_HEIGHT = 36.0f;
	static constexpr float WHEEL_RADIUS = 9.0f;
	static constexpr float RAIL_Y = ORIGIN_Y + CART_HEIGHT / 2.0f + WHEEL_RADIUS;

	NNPoleBalancerAgent(
		NNPoleBalancerAgent::GenomeCPtr&& genome,
		float cartMass, float poleMass, float poleLength, float force,
		float trackLimit, float angleLimit, float timeLimit,
		float startAngle, float startPosition);

	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window) const;

	PoleOutcome getOutcome() const { return outcome; }
	float getCartPosition() const { return cartPosition; }
	float getPoleAngle() const { return poleAngle; }
	float getPush() const { return push; }
	float getStartAngle() const { return startAngle; }
	float getStartPosition() const { return startPosition; }
	size_t getStep() const { return step; }
	sf::Vector2f getCartCentre() const { return { ORIGIN_X + cartPosition * METRE_TO_UNIT, ORIGIN_Y }; }
	sf::Vector2f getPivotPosition() const { return getCartCentre() - sf::Vector2f(0.0f, CART_HEIGHT / 2.0f); }
	sf::Vector2f getTipPosition() const { return getPivotPosition() + sf::Vector2f(std::sin(poleAngle), -std::cos(poleAngle)) * (poleLength * 2.0f * METRE_TO_UNIT); }
	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	static constexpr float G = 9.81f;

	sf::Color stateColour(std::uint8_t alpha) const;

	tbml::nn::NeuralNetwork network;
	PoleOutcome outcome = PoleOutcome::BALANCING;
	float cartMass = 1.0f;
	float poleMass = 0.1f;
	float poleLength = 0.5f;
	float force = 1;
	float trackLimit = 2.4f;
	float angleLimit = 0.21f;
	float timeLimit = 5.0f;
	float startAngle = 0.0f;
	float startPosition = 0.0f;
	tbml::Tensor netInput;
	float push = 0.0f;
	float poleAngle = 0.0f;
	float poleVelocity = 0.0f;
	float poleAcceleration = 0.0f;
	float cartPosition = 0.0f;
	float cartVelocity = 0.0f;
	float cartAcceleration = 0.0f;
	float time = 0.0f;
	size_t step = 0;
};

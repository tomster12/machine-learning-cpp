#pragma once

#include "CommonImpl.h"
#include "LanderCourse.h"

enum class LanderOutcome { FLYING, LANDED, CRASHED, LOST };

class LanderAgent : public tbml::ga::Agent<NNGenome>
{
public:
	static constexpr size_t RAY_COUNT = 7;
	static constexpr size_t INPUT_COUNT = 7 + RAY_COUNT;

	LanderAgent(LanderAgent::GenomeCPtr&& genome, std::shared_ptr<const LanderCourse> course);

	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window) const;

	LanderOutcome getOutcome() const { return outcome; }
	const std::shared_ptr<const LanderCourse>& getCourse() const { return course; }
	sf::Vector2f getPosition() const { return pos; }
	sf::Vector2f getVelocity() const { return vel; }
	float getRotation() const { return rot; }
	float getThrottle() const { return throttle; }
	float getFuel() const { return fuel; }
	size_t getStep() const { return step; }
	sf::Vector2f getNozzlePosition() const { return toWorld({ 0.0f, 12.0f }); }
	sf::Vector2f getExhaustDirection() const { return { -std::sin(rot), std::cos(rot) }; }
	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	sf::Vector2f toWorld(sf::Vector2f local) const;
	sf::Color stateColour(std::uint8_t alpha) const;
	void sense();
	void calculateFitness();
	void appendHull(sf::VertexArray& vertices, sf::Color colour, sf::Color shade) const;

	std::shared_ptr<const LanderCourse> course;
	tbml::nn::NeuralNetwork network;
	tbml::Tensor netInput;
	LanderOutcome outcome = LanderOutcome::FLYING;

	sf::Vector2f pos;
	sf::Vector2f vel;
	float rot = 0.0f;
	float angVel = 0.0f;
	float startDistance = 1.0f;
	float flightCredit = 0.0f;
	float fuel = 1.0f;
	float throttle = 0.0f;
	float steer = 0.0f;
	size_t step = 0;
};

#pragma once

#include <array>
#include "CommonImpl.h"
#include "WalkerCourse.h"

enum class WalkerOutcome { WALKING, FELL, TIMED_OUT, FINISHED };

class WalkerAgent : public tbml::ga::Agent<NNGenome>
{
public:
	static constexpr size_t HIP = 0;
	static constexpr size_t NECK = 1;
	static constexpr size_t KNEE_L = 2;
	static constexpr size_t FOOT_L = 3;
	static constexpr size_t KNEE_R = 4;
	static constexpr size_t FOOT_R = 5;
	static constexpr size_t POINT_COUNT = 6;
	static constexpr size_t JOINT_COUNT = 4;
	static constexpr size_t INPUT_COUNT = 16;
	static constexpr float PIXELS_PER_METRE = 60.0f;
	static constexpr float TIME_STEP = 1.0f / 60.0f;
	static constexpr size_t MAX_STEPS = 1000;

	WalkerAgent(WalkerAgent::GenomeCPtr&& genome, std::shared_ptr<const WalkerCourse> course);

	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window) const;

	WalkerOutcome getOutcome() const { return outcome; }
	const std::shared_ptr<const WalkerCourse>& getCourse() const { return course; }
	sf::Vector2f getPoint(size_t index) const { return points[index].pos; }
	sf::Vector2f getHipVelocity() const { return (points[HIP].pos - points[HIP].prev) / (TIME_STEP / SUBSTEPS); }
	sf::Vector2f getHeadPosition() const;
	bool getFootContact(size_t leg) const { return footContact[leg]; }
	float getTilt() const { return tilt; }
	float getDistance() const { return (bestX - WalkerCourse::START_X) / PIXELS_PER_METRE; }
	size_t getStep() const { return step; }
	const tbml::Tensor& getNetInput() const { return netInput; }
	const tbml::nn::NeuralNetwork& getBrain() const { return network; }

private:
	static constexpr int SUBSTEPS = 2;

	struct Point
	{
		sf::Vector2f pos;
		sf::Vector2f prev;
		float invMass = 1.0f;
		float radius = 3.0f;
		float friction = 0.3f;
		bool touching = false;
	};

	void sense();
	void act(const tbml::Tensor& output);
	void simulate();
	void integrate(float dt);
	void solveRod(size_t a, size_t b, float length);
	void solveJoint(size_t a, size_t pivot, size_t b, float target);
	void collide();
	void applyFriction();
	void checkOutcome();
	void calculateFitness();
	float getJointAngle(size_t joint) const;
	float getHipHeight() const;
	sf::Color stateColour(std::uint8_t alpha) const;

	std::shared_ptr<const WalkerCourse> course;
	tbml::nn::NeuralNetwork network;
	tbml::Tensor netInput;
	WalkerOutcome outcome = WalkerOutcome::WALKING;

	std::array<Point, POINT_COUNT> points;
	std::array<float, JOINT_COUNT> drive = {};
	std::array<bool, 2> footContact = {};
	float tilt = 0.0f;
	float spin = 0.0f;
	float bestX = 0.0f;
	size_t step = 0;
	size_t uprightSteps = 0;
};

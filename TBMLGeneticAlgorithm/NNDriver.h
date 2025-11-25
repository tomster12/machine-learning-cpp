#pragma once

#include "GenepoolSimulation.h"
#include "CommonImpl.h"

class Body
{
public:
	sf::Vector2f pos;
	sf::Vector2f size;
	float rot = 0.0f;

	Body() = default;
	Body(sf::Vector2f pos, sf::Vector2f size, float rot = 0.0f) : pos(pos), size(size), rot(rot) {};

	void updateShape(sf::RectangleShape& shape) const;
	void recalculateVertices();
	bool intersectBody(const Body& other) const;
	bool intersectRaycast(sf::Vector2f start, sf::Vector2f end) const;

private:
std::vector<sf::Vector2f> vertices; private:
	static bool overlapOnAxis(const std::vector<sf::Vector2f>& vertices1, const std::vector<sf::Vector2f>& vertices2, const sf::Vector2f& axis);
	static std::pair<float, float> projectVerticesOnAxis(const std::vector<sf::Vector2f>& vertices, const sf::Vector2f& axis);
};

class NNDriverGenepool;
class NNDriverAgent : public tbml::ga::Agent<NNGenome>
{
public:
	NNDriverAgent(NNDriverAgent::GenomeCPtr&& genome) : Agent(std::move(genome)) {};
	NNDriverAgent(
		NNDriverAgent::GenomeCPtr&& genome, const NNDriverGenepool* genepool,
		sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, int iterationsPerTarget);

	void initVisual();
	void setFinishedVisual();
	bool evaluate() override;
	void render(sf::RenderWindow* window) override;
	void calculateFitness();

private:

	const NNDriverGenepool* genepool = nullptr;
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
	int iterationsPerTarget = 0;

	size_t currentIteration = 0;
	size_t currentTarget = 0;
	size_t maxIterations = 0;
	size_t markedIteration = 0;
	tbml::Tensor netInput;
	float eyeHits[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float drivingSpeed = 0.0f;
	bool isFinished = false;
	int finishType = -1;
};

class NNDriverGenepool : public tbml::ga::Genepool<NNGenome, NNDriverAgent>
{
public:
	NNDriverGenepool(
		std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
		std::vector<sf::Vector2f> targets, float targetRadius, std::vector<Body> worldBodies);

	void initVisual();
	void render(sf::RenderWindow* window) override;
	bool checkWorldIntersectBody(Body& body) const;
	bool checkWorldIntersectRaycast(sf::Vector2f start, float angle, float length) const;
	float getTargetDist(sf::Vector2f pos, size_t target) const;
	float getTargetDir(sf::Vector2f pos, size_t target) const;
	float getTargetRadius() const { return targetRadius; }
	size_t getTargetCount() const { return targets.size(); }

private:
	sf::View view;
	std::vector<sf::Vector2f> targets;
	float targetRadius = 0.0f;
	std::vector<sf::CircleShape> targetShapes;
	std::vector<Body> worldBodies;
	std::vector<sf::RectangleShape> worldShapes;
};

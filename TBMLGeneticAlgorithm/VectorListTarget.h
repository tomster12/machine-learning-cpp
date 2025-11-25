#pragma once

#include "GenepoolSimulation.h"
#include "CommonImpl.h"

class VectorListTargetGenepool;
class VectorListTargetAgent : public tbml::ga::Agent<VectorListGenome>
{
public:
	VectorListTargetAgent(VectorListTargetAgent::GenomeCPtr&& genome) : Agent(std::move(genome)) {};
	VectorListTargetAgent(
		VectorListTargetAgent::GenomeCPtr&& genome, const VectorListTargetGenepool* genepool,
		sf::Vector2f startPos, float radius, float moveAcc);

	void initVisual();
	bool evaluate() override;
	void render(sf::RenderWindow* window) override;
	float calculateDist();
	float calculateFitness();

private:
	const VectorListTargetGenepool* genepool = nullptr;
	bool isVisualInit = false;
	sf::CircleShape shape;
	sf::Vector2f startPos;
	sf::Vector2f pos;
	float moveAcc = 0.0f;
	float radius = 0.0f;
	int currentIndex = -1;
};

class VectorListTargetGenepool : public tbml::ga::Genepool<VectorListGenome, VectorListTargetAgent>
{
public:
	VectorListTargetGenepool(
		std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
		sf::Vector2f targetPos, float targetRadius);

	void initVisual();
	void render(sf::RenderWindow* window) override;
	sf::Vector2f getTargetPos() const;
	float getTargetRadius() const;

protected:
	sf::CircleShape target;
	sf::Vector2f targetPos;
	float targetRadius = 0.0f;
};

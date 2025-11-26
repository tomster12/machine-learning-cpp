#pragma once

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
	void render(sf::RenderWindow* window);
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

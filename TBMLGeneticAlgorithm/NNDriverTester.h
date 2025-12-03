#pragma once

#include "CommonImpl.h"
#include "Body.h"

class NNDriverTester
{
public:
	NNDriverTester(sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, float eyeSpread, int iterationsPerTarget);
	bool update();
	void render(sf::RenderWindow* window);

private:
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

	tbml::Tensor netInput;
	float eyeHits[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float drivingSpeed = 0.0f;
};

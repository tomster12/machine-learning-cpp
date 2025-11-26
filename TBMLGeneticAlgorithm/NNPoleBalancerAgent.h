//#pragma once
//
//#include "GenepoolSimulation.h"
//#include "CommonImpl.h"
//
//// https://researchbank.swinburne.edu.au/file/62a8df69-4a2c-407f-8040-5ac533fc2787/1/PDF%20(12%20pages).pdf
//class NNPoleBalancerAgent : public tbml::ga::Agent<NNGenome>
//{
//public:
//	NNPoleBalancerAgent(NNPoleBalancerAgent::GenomeCPtr&& genome) : Agent(std::move(genome)) {};
//	NNPoleBalancerAgent(
//		NNPoleBalancerAgent::GenomeCPtr&& genome,
//		float cartMass, float poleMass, float poleLength, float force,
//		float trackLimit, float angleLimit, float timeLimit);
//
//	void initVisual();
//	bool evaluate() override;
//	void render(sf::RenderWindow* window) override;
//
//private:
//	const float G = 9.81f;
//	const float TIME_STEP = 0.02f;
//	const float METRE_TO_UNIT = 200.0f;
//
//	bool isVisualInit = false;
//	sf::RectangleShape cartShape;
//	sf::RectangleShape poleShape;
//	float cartMass = 1.0f;
//	float poleMass = 0.1f;
//	float poleLength = 0.5f;
//	float force = 1;
//	float trackLimit = 2.4f;
//	float angleLimit = 0.21f;
//	float timeLimit = 5.0f;
//	tbml::Tensor netInput;
//	float poleAngle = 0.0f;
//	float poleVelocity = 0.0f;
//	float poleAcceleration = 0.0f;
//	float cartPosition = 0.0f;
//	float cartVelocity = 0.0f;
//	float cartAcceleration = 0.0f;
//	float time = 0.0f;
//};

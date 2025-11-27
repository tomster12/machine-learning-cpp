#include "stdafx.h"
#include "NNDriverGenepool.h"
#include "NNDriverAgent.h"

NNDriverAgent::NNDriverAgent(
	NNDriverAgent::GenomeCPtr&& genome, const NNDriverGenepool* genepool,
	sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, int iterationsPerTarget)
	: Agent(std::move(genome)), genepool(genepool),
	maxDrivingSpeed(maxDrivingSpeed), drivingAcc(drivingAcc), steeringSpeed(steeringSpeed), moveDrag(moveDrag), eyeLength(eyeLength), iterationsPerTarget(iterationsPerTarget),
	maxIterations(iterationsPerTarget)
{
	mainBody = Body(startPos, sf::Vector2f(40.0f, 20.0f), 1.5f * 3.14159265f);
}

void NNDriverAgent::initVisual()
{
	if (isVisualInit) return;

	eyeColourHit = sf::Color(200, 100, 100, 20);
	eyeColourMiss = sf::Color(100, 100, 100, 20);

	mainShape.setFillColor(sf::Color::Transparent);
	mainShape.setOutlineColor(sf::Color(255, 255, 255, 140));
	mainShape.setOutlineThickness(1.0f);
	mainShape.setSize(mainBody.size);
	mainShape.setOrigin({ mainBody.size.x / 2.0f, mainBody.size.y / 2.0f });

	eyeShape.setFillColor(eyeColourMiss);
	eyeShape.setSize(sf::Vector2f(eyeLength, 3.0f));
	eyeShape.setOrigin({ 0, 1.5f });

	if (isFinished) this->setFinishedVisual();

	isVisualInit = true;
}

void NNDriverAgent::setFinishedVisual()
{
	if (!isVisualInit) return;

	// Set main shape based on finish type
	// 0: Collided, 1: Max iterations, 2: Reached target
	if (finishType == 0) mainShape.setOutlineColor(sf::Color(200, 100, 100, 60));
	else if (finishType == 1) mainShape.setOutlineColor(sf::Color(200, 200, 100, 60));
	else if (finishType == 2) mainShape.setOutlineColor(sf::Color(100, 200, 100, 60));

	// Set eye shapes to transparent
	eyeShape.setFillColor(sf::Color::Transparent);
}

bool NNDriverAgent::evaluate()
{
	if (isFinished) return true;

	// Finish 0: Collided with world
	mainBody.recalculateVertices();
	if (genepool->checkWorldIntersectBody(mainBody))
	{
		isFinished = true;
		finishType = 0;
		this->calculateFitness();
		setFinishedVisual();
		return true;
	}

	// Raycast with eye shapes (-45, -20, 0, 20, 45)
	sf::Vector2f pos = mainBody.pos + sf::Vector2f(20.0f * std::cos(mainBody.rot), 20.0f * std::sin(mainBody.rot));
	for (int i = 0; i < 5; i++)
	{
		float angle = mainBody.rot + (i - 2) * 0.2f * 3.14159265f;
		eyeHits[i] = genepool->checkWorldIntersectRaycast(pos, angle, eyeLength) ? 1.0f : 0.0f;
	}

	// Calculate with brain (bias, eyes, speed, angle, angle diff)
	float rotDiff = genepool->getTargetDir(mainBody.pos, currentTarget) - mainBody.rot;
	netInput.set({ 1, 8 }, { eyeHits[0], eyeHits[1], eyeHits[2], eyeHits[3], eyeHits[4], drivingSpeed, mainBody.rot, rotDiff });
	genome->getNetwork().propogateMut(netInput);

	// Update position, angle, speed
	mainBody.rot += netInput(0, 0) * steeringSpeed;
	drivingSpeed += netInput(0, 1) * drivingAcc;
	mainBody.rot = std::fmod(mainBody.rot + 2.0f * 3.14159265f, 2.0f * 3.14159265f);
	drivingSpeed = std::max(0.0f, std::min(maxDrivingSpeed, drivingSpeed * moveDrag));
	mainBody.pos.x += std::cos(mainBody.rot) * drivingSpeed * (1.0f / 60.0f);
	mainBody.pos.y += std::sin(mainBody.rot) * drivingSpeed * (1.0f / 60.0f);
	currentIteration++;

	// Finish 1: Max iterations
	if (currentIteration >= maxIterations)
	{
		isFinished = true;
		finishType = 1;
		this->calculateFitness();
		setFinishedVisual();
		return true;
	}

	// Finish 2: Reached target
	if (genepool->getTargetDist(mainBody.pos, currentTarget) < genepool->getTargetRadius())
	{
		currentTarget++;
		maxIterations += iterationsPerTarget;
		markedIteration = currentIteration;

		if (currentTarget == genepool->getTargetCount())
		{
			isFinished = true;
			finishType = 2;
			this->calculateFitness();
			setFinishedVisual();
			return true;
		}
	}

	return isFinished;
}

void NNDriverAgent::render(sf::RenderWindow* window)
{
	if (!isVisualInit) this->initVisual();

	mainBody.updateShape(mainShape);
	window->draw(mainShape);

	if (!isFinished)
	{
		sf::Vector2f pos = mainBody.pos + sf::Vector2f(20.0f * std::cos(mainBody.rot), 20.0f * std::sin(mainBody.rot));
		for (int i = 0; i < 5; i++)
		{
			float angle = mainBody.rot + (i - 2) * 0.2f;
			eyeShape.setPosition(pos);
			eyeShape.setRotation(sf::radians(angle));
			eyeShape.setFillColor(eyeHits[i] > 0.5f ? eyeColourHit : eyeColourMiss);
			window->draw(eyeShape);
		}
	}
}

void NNDriverAgent::calculateFitness()
{
	// Reward targets reached (2 points)
	fitness = currentTarget * 2.0f;

	// Reward speed (up to 1 points)
	// Expect each target to be reached in 100 iterations
	if (currentTarget > 0)
	{
		fitness += std::min(1.0f, 1.0f / std::max(1.0f, markedIteration / (currentTarget * 100.0f)));
	}

	// Reward distance to next target (up to 1 point)
	if (currentTarget < genepool->getTargetCount())
	{
		float dist = genepool->getTargetDist(mainBody.pos, currentTarget);
		fitness += 1.0f / std::max(1.0f, dist / 20.0f);
	}
}

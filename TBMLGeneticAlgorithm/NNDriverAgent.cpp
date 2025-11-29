#include "stdafx.h"
#include "NNDriverGenepool.h"
#include "NNDriverAgent.h"

NNDriverAgent::NNDriverAgent(
	NNDriverAgent::GenomeCPtr&& genome, const NNDriverGenepool* genepool,
	sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, float eyeSpread, int iterationsPerTarget)
	: Agent(std::move(genome)), genepool(genepool), network(this->genome->copyNetwork()), netInput({ 1, 8 }, 0.0f),
	maxDrivingSpeed(maxDrivingSpeed), drivingAcc(drivingAcc), steeringSpeed(steeringSpeed), moveDrag(moveDrag), eyeLength(eyeLength), eyeSpread(eyeSpread), iterationsPerTarget(iterationsPerTarget),
	maxIterations(iterationsPerTarget), fitnessText(global::font)
{
	mainBody = Body(startPos, sf::Vector2f(40.0f, 20.0f), 1.5f * 3.14159265f);
}

void NNDriverAgent::initVisual()
{
	if (isVisualInit) return;

	eyeColourHit = sf::Color(200, 100, 100, 60);
	eyeColourMiss = sf::Color(100, 100, 100, 20);

	mainShape.setFillColor(sf::Color::Transparent);
	mainShape.setOutlineColor(sf::Color(255, 255, 255, 140));
	mainShape.setOutlineThickness(1.0f);
	mainShape.setSize(mainBody.size);
	mainShape.setOrigin({ mainBody.size.x / 2.0f, mainBody.size.y / 2.0f });

	eyeShape.setFillColor(eyeColourMiss);
	eyeShape.setSize(sf::Vector2f(eyeLength, 3.0f));
	eyeShape.setOrigin({ 0, 1.5f });

	fitnessText.setCharacterSize(12);
	fitnessText.setFillColor(sf::Color::White);

	if (isFinished) this->setFinishedVisual();

	isVisualInit = true;
}

void NNDriverAgent::setFinishedVisual()
{
	if (!isVisualInit) return;

	// Set main shape based on finish type
	// 0: Collided, 1: Max iterations, 2: Reached target
	if (finishType == 0) mainShape.setOutlineColor(sf::Color(200, 100, 100, 150));
	else if (finishType == 1) mainShape.setOutlineColor(sf::Color(200, 200, 100, 150));
	else if (finishType == 2) mainShape.setOutlineColor(sf::Color(100, 200, 100, 150));

	// Set eye shapes to transparent
	eyeShape.setFillColor(sf::Color::Transparent);
}

bool NNDriverAgent::evaluate()
{
	if (isFinished) return true;

	this->calculateFitness();

	// Finish 0: Collided with world
	mainBody.recalculateVertices();
	if (genepool->checkWorldIntersectBody(mainBody))
	{
		isFinished = true;
		finishType = 0;
		setFinishedVisual();
		return true;
	}

	// Raycast with eye shapes
	sf::Vector2f pos = mainBody.pos;
	for (int i = 0; i < 5; i++)
	{
		float angle = mainBody.rot + (i - 2) * eyeSpread;
		eyeHits[i] = genepool->checkWorldIntersectRaycast(pos, angle, eyeLength) ? 1.0f : 0.0f;
	}

	// Calculate with brain (bias, eyes, speed, angle, angle diff)
	float rotDiff = genepool->getTargetDirDiff(mainBody, currentTarget);
	netInput.setData({ eyeHits[0], eyeHits[1], eyeHits[2], eyeHits[3], eyeHits[4], drivingSpeed, mainBody.rot, rotDiff });
	const tbml::Tensor* netOutput = network.propogatePtr(&netInput);

	// Update position, angle, speed
	mainBody.rot += netOutput->at(0, 0) * steeringSpeed;
	drivingSpeed += netOutput->at(0, 1) * drivingAcc;
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
		setFinishedVisual();
		return true;
	}

	// Finish 2: Reached target
	if (genepool->getTargetDist(mainBody.pos, currentTarget) < 0)
	{
		currentTarget++;
		maxIterations += iterationsPerTarget;
		reachedTargetIteration = currentIteration;

		if (currentTarget == genepool->getTargetCount())
		{
			isFinished = true;
			finishType = 2;
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
		sf::Vector2f pos = mainBody.pos;
		for (int i = 0; i < 5; i++)
		{
			float angle = mainBody.rot + (i - 2) * eyeSpread;
			eyeShape.setPosition(pos);
			eyeShape.setRotation(sf::radians(angle));
			eyeShape.setFillColor(eyeHits[i] > 0.5f ? eyeColourHit : eyeColourMiss);
			window->draw(eyeShape);
		}
	}

	fitnessText.setString(std::to_string(fitness).substr(0, 5));
	fitnessText.setOrigin(fitnessText.getLocalBounds().getCenter());
	fitnessText.setPosition(mainBody.pos);
	fitnessText.setRotation(sf::radians(mainBody.rot));
	window->draw(fitnessText);
}

void NNDriverAgent::calculateFitness()
{
	fitness = 0;

	// Reward targets reached (5 points)
	const float targetFitness = currentTarget * 5.0f;
	fitness += targetFitness;

	// Reward speed of reaching targets (up to 2 points)
	const float averageIterationsPerTarget = static_cast<float>(reachedTargetIteration) / static_cast<float>(currentTarget);
	const float speedFitness = (1.0f - std::min(1.0f, averageIterationsPerTarget / static_cast<float>(iterationsPerTarget))) * 2.0f;
	fitness += speedFitness;

	// Reward distance to next target (up to 2 points)
	if (currentTarget < genepool->getTargetCount())
	{
		float dist = genepool->getTargetDist(mainBody.pos, currentTarget);
		fitness += (1.0f / std::max(1.0f, dist / 20.0f)) * 2.0f;
	}

	// Penalize for collisions (up to -3 points)
	if (isFinished && finishType == 0) fitness -= 3.0f;
	if (fitness < 0.0f) fitness = 0.0f;

	// Square fitness
	fitness = fitness * fitness;
}

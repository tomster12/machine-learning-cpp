#include "stdafx.h"
#include "NNDriverGenepool.h"
#include "NNDriverAgent.h"
#include "UITheme.h"

NNDriverAgent::NNDriverAgent(
	NNDriverAgent::GenomeCPtr&& genome, const NNDriverGenepool* genepool,
	sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, float eyeSpread, int iterationsPerTarget)
	: Agent(std::move(genome)), genepool(genepool), network(this->genome->copyNetwork()), netInput({ 1, 8 }, 0.0f),
	maxDrivingSpeed(maxDrivingSpeed), drivingAcc(drivingAcc), steeringSpeed(steeringSpeed), moveDrag(moveDrag), eyeLength(eyeLength), eyeSpread(eyeSpread), iterationsPerTarget(iterationsPerTarget),
	maxIterations(iterationsPerTarget)
{
	mainBody = Body(startPos, sf::Vector2f(40.0f, 20.0f), 1.5f * 3.14159265f);
}

void NNDriverAgent::initVisual()
{
	if (isVisualInit) return;

	// Only the elite is drawn with shapes, the rest of the pack is batched into a vertex array
	eyeColourHit = sf::Color(theme::NEGATIVE.r, theme::NEGATIVE.g, theme::NEGATIVE.b, 200);
	eyeColourMiss = sf::Color(255, 255, 255, 60);

	mainShape.setSize(mainBody.size);
	mainShape.setOrigin({ mainBody.size.x / 2.0f, mainBody.size.y / 2.0f });

	eyeShape.setFillColor(eyeColourMiss);
	eyeShape.setSize(sf::Vector2f(eyeLength, 2.0f));
	eyeShape.setOrigin({ 0, 1.0f });

	isVisualInit = true;
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
			return true;
		}
	}

	return isFinished;
}

void NNDriverAgent::appendPackVertices(sf::VertexArray& vertices) const
{
	// Colour shows state, opacity shows role: alive cars are clearer than finished ones (the elite is drawn separately, fully opaque)
	const sf::Color colour = stateColour(isFinished ? 70 : 130);

	const float c = std::cos(mainBody.rot);
	const float s = std::sin(mainBody.rot);
	const float w = mainBody.size.x / 2.0f;
	const float h = mainBody.size.y / 2.0f;
	auto corner = [&](float lx, float ly) { return sf::Vector2f(mainBody.pos.x + lx * c - ly * s, mainBody.pos.y + lx * s + ly * c); };

	const sf::Vertex rearLeft{ corner(-w, -h), colour };
	const sf::Vertex frontLeft{ corner(w, -h), colour };
	const sf::Vertex frontRight{ corner(w, h), colour };
	const sf::Vertex rearRight{ corner(-w, h), colour };

	vertices.append(rearLeft);
	vertices.append(frontLeft);
	vertices.append(frontRight);
	vertices.append(rearLeft);
	vertices.append(frontRight);
	vertices.append(rearRight);
}

void NNDriverAgent::renderElite(sf::RenderWindow* window)
{
	if (!isVisualInit) this->initVisual();

	if (!isFinished)
	{
		for (int i = 0; i < 5; i++)
		{
			float angle = mainBody.rot + (i - 2) * eyeSpread;
			eyeShape.setPosition(mainBody.pos);
			eyeShape.setRotation(sf::radians(angle));
			eyeShape.setFillColor(eyeHits[i] > 0.5f ? eyeColourHit : eyeColourMiss);
			window->draw(eyeShape);
		}
	}

	mainShape.setFillColor(stateColour(255));
	mainBody.updateShape(mainShape);
	window->draw(mainShape);

	// Fitness readout on a small plate above the car, kept upright
	const std::string label = global::formatFloat(fitness, 1);
	const float plateWidth = theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
	const sf::Vector2f centre(mainBody.pos.x, mainBody.pos.y - 28.0f);
	theme::drawRect(window, { centre.x - plateWidth / 2.0f, centre.y - 9.0f }, { plateWidth, 18.0f }, theme::PANEL_FILL);
	theme::drawText(window, label, centre, theme::TEXT_SMALL, theme::TEXT_PRIMARY, theme::Align::CENTER);
}

sf::Color NNDriverAgent::stateColour(std::uint8_t alpha) const
{
	// 0: Collided, 1: Max iterations, 2: Reached all targets
	sf::Color base = theme::CAR_ALIVE;
	if (isFinished)
	{
		if (finishType == 0) base = theme::CAR_CRASHED;
		else if (finishType == 1) base = theme::CAR_TIMED_OUT;
		else base = theme::CAR_COMPLETED;
	}
	return sf::Color(base.r, base.g, base.b, alpha);
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

#include "stdafx.h"
#include "NNDriverAgent.h"
#include "UITheme.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float TIME_STEP = 1.0f / 60.0f;
	constexpr float STEER_SMOOTHING = 0.25f;
	constexpr float TARGET_DIST_SCALE = 600.0f;
}

NNDriverAgent::NNDriverAgent(
	NNDriverAgent::GenomeCPtr&& genome, std::shared_ptr<const DriverCourse> course,
	float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, float eyeSpread, int iterationsPerTarget)
	: Agent(std::move(genome)), course(std::move(course)), network(this->genome->copyNetwork()), netInput({ 1, INPUT_COUNT }, 0.0f),
	maxDrivingSpeed(maxDrivingSpeed), steeringSpeed(steeringSpeed), drivingAcc(drivingAcc), moveDrag(moveDrag), eyeLength(eyeLength), eyeSpread(eyeSpread), iterationsPerTarget(iterationsPerTarget),
	maxIterations(iterationsPerTarget)
{
	mainBody = Body(this->course->getStartPos(), sf::Vector2f(40.0f, 20.0f), this->course->getStartRot());
}

bool NNDriverAgent::evaluate()
{
	if (isFinished) return true;

	this->calculateFitness();

	// Finish 0: Collided with world
	mainBody.recalculateVertices();
	if (course->intersectsBody(mainBody))
	{
		isFinished = true;
		finishType = 0;
		return true;
	}

	// Distance sensors, closer walls read higher
	const sf::Vector2f pos = mainBody.pos;
	for (size_t i = 0; i < EYE_COUNT; i++)
	{
		const float angle = mainBody.rot + (static_cast<float>(i) - 2.0f) * eyeSpread;
		eyeHits[i] = 1.0f - course->castRay(pos, angle, eyeLength);
	}

	// Calculate with brain (eyes, speed, distance to target, angle to target), all scaled to roughly -1 to 1
	const float targetDist = course->getTargetDist(pos, currentTarget);
	const float rotDiff = course->getTargetDirDiff(pos, mainBody.rot, currentTarget);
	netInput.setData({ eyeHits[0], eyeHits[1], eyeHits[2], eyeHits[3], eyeHits[4], drivingSpeed / maxDrivingSpeed, std::min(1.0f, targetDist / TARGET_DIST_SCALE), rotDiff / PI });
	const tbml::Tensor* netOutput = network.propogatePtr(&netInput);
	throttle = netOutput->at(0, 1);

	// The wheel eases towards the requested angle, and bites harder the faster the car is going
	steer += (netOutput->at(0, 0) - steer) * STEER_SMOOTHING;
	const float grip = 0.3f + 0.7f * std::min(1.0f, drivingSpeed / (0.4f * maxDrivingSpeed));

	// Update position, angle, speed
	mainBody.rot += steer * steeringSpeed * grip;
	drivingSpeed += throttle * drivingAcc;
	mainBody.rot = std::fmod(mainBody.rot + 2.0f * PI, 2.0f * PI);
	drivingSpeed = std::max(0.0f, std::min(maxDrivingSpeed, drivingSpeed * moveDrag));
	mainBody.pos.x += std::cos(mainBody.rot) * drivingSpeed * TIME_STEP;
	mainBody.pos.y += std::sin(mainBody.rot) * drivingSpeed * TIME_STEP;
	currentIteration++;

	// Finish 1: Max iterations
	if (currentIteration >= maxIterations)
	{
		isFinished = true;
		finishType = 1;
		return true;
	}

	// Finish 2: Reached target
	if (course->getTargetDist(mainBody.pos, currentTarget) < 0)
	{
		currentTarget++;
		maxIterations += iterationsPerTarget;
		reachedTargetIteration = currentIteration;

		if (currentTarget == course->getTargetCount())
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

	const float w = mainBody.size.x / 2.0f;
	const float h = mainBody.size.y / 2.0f;
	theme::appendQuad(vertices, this->toWorld({ -w, -h }), this->toWorld({ w, -h }), this->toWorld({ w, h }), this->toWorld({ -w, h }), colour);
}

void NNDriverAgent::renderElite(sf::RenderWindow* window) const
{
	const sf::Color body = stateColour(255);
	const sf::Color dark(26, 28, 32);
	const sf::Color cabin(body.r * 55 / 100, body.g * 55 / 100, body.b * 55 / 100);
	const bool running = !isFinished;
	const float halfWidth = mainBody.size.x / 2.0f;
	const float halfHeight = mainBody.size.y / 2.0f;

	sf::VertexArray vertices(sf::PrimitiveType::Triangles);
	auto rect = [&](float x0, float y0, float x1, float y1, sf::Color colour)
	{
		theme::appendQuad(vertices, this->toWorld({ x0, y0 }), this->toWorld({ x1, y0 }), this->toWorld({ x1, y1 }), this->toWorld({ x0, y1 }), colour);
	};
	auto wheel = [&](float x, float y, float angle)
	{
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		auto corner = [&](float lx, float ly) { return this->toWorld({ x + lx * c - ly * s, y + lx * s + ly * c }); };
		theme::appendQuad(vertices, corner(-6.0f, -2.5f), corner(6.0f, -2.5f), corner(6.0f, 2.5f), corner(-6.0f, 2.5f), dark);
	};

	// Soft drop shadow under the car
	const sf::Vector2f offset(3.0f, 5.0f);
	theme::appendQuad(vertices,
		this->toWorld({ -halfWidth, -halfHeight }) + offset, this->toWorld({ halfWidth, -halfHeight }) + offset,
		this->toWorld({ halfWidth, halfHeight }) + offset, this->toWorld({ -halfWidth, halfHeight }) + offset, sf::Color(0, 0, 0, 70));

	if (running)
	{
		// Headlight beams
		for (int side = -1; side <= 1; side += 2)
		{
			const float y = static_cast<float>(side) * (halfHeight - 3.0f);
			vertices.append({ this->toWorld({ halfWidth, y }), sf::Color(255, 244, 200, 46) });
			vertices.append({ this->toWorld({ halfWidth + 70.0f, y - 16.0f }), sf::Color(255, 244, 200, 0) });
			vertices.append({ this->toWorld({ halfWidth + 70.0f, y + 16.0f }), sf::Color(255, 244, 200, 0) });
		}

		// The distance sensors that feed the network, ending where they hit a wall
		for (size_t i = 0; i < EYE_COUNT; i++)
		{
			const float angle = mainBody.rot + (static_cast<float>(i) - 2.0f) * eyeSpread;
			const sf::Vector2f end = mainBody.pos + sf::Vector2f(std::cos(angle), std::sin(angle)) * (eyeLength * (1.0f - eyeHits[i]));
			const bool hit = eyeHits[i] > 0.0f;
			theme::appendLine(vertices, mainBody.pos, end, 2.0f, hit ? sf::Color(theme::NEGATIVE.r, theme::NEGATIVE.g, theme::NEGATIVE.b, 200) : sf::Color(255, 255, 255, 60));
			if (hit) theme::appendDisc(vertices, end, 3.0f, theme::NEGATIVE, 8);
		}
	}

	// Wheels with the front pair turned into the corner
	wheel(-12.0f, -halfHeight, 0.0f);
	wheel(-12.0f, halfHeight, 0.0f);
	wheel(12.0f, -halfHeight, steer * 0.5f);
	wheel(12.0f, halfHeight, steer * 0.5f);

	// Body, cabin and windscreen
	rect(-halfWidth, -halfHeight, halfWidth, halfHeight, body);
	rect(-10.0f, -halfHeight + 3.0f, 4.0f, halfHeight - 3.0f, cabin);
	rect(4.0f, -halfHeight + 3.0f, 9.0f, halfHeight - 3.0f, sf::Color(160, 200, 230, 235));

	// Headlights up front, brake lights at the back glowing while the car is slowing
	const sf::Color lamp = running ? sf::Color(255, 244, 200) : sf::Color(120, 120, 110);
	rect(halfWidth - 3.0f, -halfHeight + 1.0f, halfWidth, -halfHeight + 5.0f, lamp);
	rect(halfWidth - 3.0f, halfHeight - 5.0f, halfWidth, halfHeight - 1.0f, lamp);
	const sf::Color brake = running && throttle < -0.1f ? sf::Color(255, 70, 60) : sf::Color(150, 40, 36);
	rect(-halfWidth, -halfHeight + 1.0f, -halfWidth + 3.0f, -halfHeight + 5.0f, brake);
	rect(-halfWidth, halfHeight - 5.0f, -halfWidth + 3.0f, halfHeight - 1.0f, brake);
	window->draw(vertices);

	// Fitness readout on a small plate above the car, kept upright
	const std::string label = global::formatFloat(fitness, 1);
	const float plateWidth = theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
	const sf::Vector2f centre(mainBody.pos.x, mainBody.pos.y - 28.0f);
	theme::drawRect(window, { centre.x - plateWidth / 2.0f, centre.y - 9.0f }, { plateWidth, 18.0f }, theme::PANEL_FILL);
	theme::drawText(window, label, centre, theme::TEXT_SMALL, theme::TEXT_PRIMARY, theme::Align::CENTER);
}

sf::Vector2f NNDriverAgent::toWorld(sf::Vector2f local) const
{
	const float c = std::cos(mainBody.rot);
	const float s = std::sin(mainBody.rot);
	return { mainBody.pos.x + local.x * c - local.y * s, mainBody.pos.y + local.x * s + local.y * c };
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
	if (currentTarget > 0)
	{
		const float averageIterationsPerTarget = static_cast<float>(reachedTargetIteration) / static_cast<float>(currentTarget);
		const float speedFitness = (1.0f - std::min(1.0f, averageIterationsPerTarget / static_cast<float>(iterationsPerTarget))) * 2.0f;
		fitness += speedFitness;
	}

	// Reward progress along the leg to the next target (up to 2 points)
	if (currentTarget < course->getTargetCount())
	{
		const sf::Vector2f from = currentTarget == 0 ? course->getStartPos() : course->getTarget(currentTarget - 1);
		const sf::Vector2f to = course->getTarget(currentTarget);
		const float legLength = std::max((to - from).length(), 1.0f);
		fitness += std::clamp(1.0f - (to - mainBody.pos).length() / legLength, 0.0f, 1.0f) * 2.0f;
	}

	// Penalize for collisions (up to -3 points)
	if (isFinished && finishType == 0) fitness -= 3.0f;
	if (fitness < 0.0f) fitness = 0.0f;

	// Square fitness
	fitness = fitness * fitness;
}

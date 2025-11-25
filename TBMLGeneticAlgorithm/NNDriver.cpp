#include "stdafx.h"
#include "NNDriver.h"
#include "CommonImpl.h"
#include "Tensor.h"

void Body::updateShape(sf::RectangleShape& shape) const
{
	shape.setPosition(pos);
	shape.setRotation(sf::degrees(rot));
}

void Body::recalculateVertices()
{
	float c = std::cos(rot);
	float s = std::sin(rot);
	float w = size.x / 2.0f;
	float h = size.y / 2.0f;
	vertices = {
		{ pos.x + w * c - h * s, pos.y + w * s + h * c },
		{ pos.x - w * c - h * s, pos.y - w * s + h * c },
		{ pos.x - w * c + h * s, pos.y - w * s - h * c },
		{ pos.x + w * c + h * s, pos.y + w * s - h * c }
	};
}

std::pair<float, float> Body::projectVerticesOnAxis(const std::vector<sf::Vector2f>& vertices, const sf::Vector2f& axis)
{
	// Find min and max projection of all vertices on axis
	float min = (vertices[0].x * axis.x + vertices[0].y * axis.y);
	float max = min;
	for (const auto& vertex : vertices)
	{
		float projection = (vertex.x * axis.x + vertex.y * axis.y);
		if (projection < min) min = projection;
		if (projection > max) max = projection;
	}
	return { min, max };
}

bool Body::overlapOnAxis(const std::vector<sf::Vector2f>& vertices1, const std::vector<sf::Vector2f>& vertices2, const sf::Vector2f& axis)
{
	// Overlap on axis if projection min and max overlap
	auto pair1 = projectVerticesOnAxis(vertices1, axis);
	auto pair2 = projectVerticesOnAxis(vertices2, axis);
	return !(pair1.second < pair2.first || pair2.second < pair1.first);
}

bool Body::intersectBody(const Body& other) const
{
	// Get all axes as all edges from both rectangles
	std::vector<sf::Vector2f> axes;
	for (size_t i = 0; i < 4; ++i)
	{
		sf::Vector2f edge1 = vertices[i] - vertices[(i + 1) % 4];
		sf::Vector2f edge2 = other.vertices[i] - other.vertices[(i + 1) % 4];
		axes.push_back(sf::Vector2f(-edge1.y, edge1.x));
		axes.push_back(sf::Vector2f(-edge2.y, edge2.x));
	}

	// Collision if there is no axis of separation
	for (const auto& axis : axes)
	{
		if (!overlapOnAxis(vertices, other.vertices, axis)) return false;
	}
	return true;
}

bool Body::intersectRaycast(sf::Vector2f start, sf::Vector2f end) const
{
	// Check if ray intersects any edge of the body
	for (size_t i = 0; i < 4; ++i)
	{
		sf::Vector2f edge = vertices[(i + 1) % 4] - vertices[i];
		sf::Vector2f normal = sf::Vector2f(-edge.y, edge.x);
		float startProj = (start.x * normal.x + start.y * normal.y);
		float endProj = (end.x * normal.x + end.y * normal.y);
		float minProj = std::min(startProj, endProj);
		float maxProj = std::max(startProj, endProj);
		auto pair = projectVerticesOnAxis(vertices, normal);
		if (pair.second < minProj || pair.first > maxProj) return false;
	}
	return true;
}

NNDriverAgent::NNDriverAgent(
	NNDriverAgent::GenomeCPtr&& genome, const NNDriverGenepool* genepool,
	sf::Vector2f startPos, float maxDrivingSpeed, float drivingAcc, float steeringSpeed, float moveDrag, float eyeLength, int iterationsPerTarget)
	: Agent(std::move(genome)), genepool(genepool),
	maxDrivingSpeed(maxDrivingSpeed), drivingAcc(drivingAcc), steeringSpeed(steeringSpeed), moveDrag(moveDrag), eyeLength(eyeLength), iterationsPerTarget(iterationsPerTarget),
	maxIterations(iterationsPerTarget)
{
	// Initialize body
	mainBody = Body(startPos, sf::Vector2f(40.0f, 20.0f), 1.5f * 3.14159265f);
}

void NNDriverAgent::initVisual()
{
	if (isVisualInit) return;

	// Initialize eye colors
	eyeColourHit = sf::Color(200, 100, 100, 120);
	eyeColourMiss = sf::Color(100, 100, 100, 120);

	// Set up body shape
	mainShape.setFillColor(sf::Color::Transparent);
	mainShape.setOutlineColor(sf::Color(255, 255, 255, 120));
	mainShape.setOutlineThickness(1.0f);
	mainShape.setSize(mainBody.size);
	mainShape.setOrigin({ mainBody.size.x / 2.0f, mainBody.size.y / 2.0f });

	// Set up eye shape
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
	netInput.setData({ 1, 8 }, { eyeHits[0], eyeHits[1], eyeHits[2], eyeHits[3], eyeHits[4], drivingSpeed, mainBody.rot, rotDiff });
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

	// Draw main shape
	mainBody.updateShape(mainShape);
	window->draw(mainShape);

	// Draw eye shape for each eye
	if (!isFinished)
	{
		sf::Vector2f pos = mainBody.pos + sf::Vector2f(20.0f * std::cos(mainBody.rot), 20.0f * std::sin(mainBody.rot));
		for (int i = 0; i < 5; i++)
		{
			float angle = mainBody.rot + (i - 2) * 0.2f * 3.14159265f;
			eyeShape.setPosition(pos);
			eyeShape.setRotation(sf::degrees(angle));
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

NNDriverGenepool::NNDriverGenepool(
	std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn,
	std::vector<sf::Vector2f> targets, float targetRadius, std::vector<Body> worldBodies)
	: Genepool(createGenomeFn, createAgentFn),
	targets(targets), targetRadius(targetRadius), worldBodies(worldBodies)
{
	this->initVisual();
}

void NNDriverGenepool::initVisual()
{
	// Set up target shapes
	for (const auto& target : targets)
	{
		sf::CircleShape shape;
		shape.setPosition(target);
		shape.setFillColor(sf::Color::Transparent);
		shape.setOutlineColor(sf::Color::Green);
		shape.setOutlineThickness(1.0f);
		shape.setRadius(targetRadius);
		shape.setOrigin({ targetRadius, targetRadius });
		targetShapes.push_back(shape);
	}

	// Set up world shapes
	for (auto& body : worldBodies)
	{
		sf::RectangleShape shape;
		shape.setFillColor(sf::Color::Transparent);
		shape.setOutlineColor(sf::Color::White);
		shape.setOutlineThickness(1.0f);
		shape.setSize(body.size);
		shape.setOrigin({ body.size.x / 2.0f, body.size.y / 2.0f });
		body.updateShape(shape);
		body.recalculateVertices();
		worldShapes.push_back(shape);
	}
}

void NNDriverGenepool::render(sf::RenderWindow* window)
{
	Genepool::render(window);
	if (!this->showVisuals) return;

	// Draw target shapes
	for (const auto& shape : targetShapes) window->draw(shape);

	// Draw world shapes
	for (const auto& shape : worldShapes) window->draw(shape);
}

bool NNDriverGenepool::checkWorldIntersectBody(Body& body) const
{
	// Check if body intersects any other body
	for (const auto& other : worldBodies)
	{
		if (body.intersectBody(other)) return true;
	}
	return false;
}

bool NNDriverGenepool::checkWorldIntersectRaycast(sf::Vector2f start, float angle, float length) const
{
	// Get end point of raycast
	sf::Vector2f end = start + sf::Vector2f(std::cos(angle), std::sin(angle)) * length;

	// Check if ray intersects any body
	for (const auto& body : worldBodies)
	{
		if (body.intersectRaycast(start, end)) return true;
	}
	return false;
}

float NNDriverGenepool::getTargetDist(sf::Vector2f pos, size_t target) const
{
	float dx = targets[target].x - pos.x;
	float dy = targets[target].y - pos.y;
	return sqrt(dx * dx + dy * dy) - targetRadius;
}

float NNDriverGenepool::getTargetDir(sf::Vector2f pos, size_t target) const
{
	float dx = targets[target].x - pos.x;
	float dy = targets[target].y - pos.y;
	return atan2(dy, dx);
}

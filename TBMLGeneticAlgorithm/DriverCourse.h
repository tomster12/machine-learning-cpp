#pragma once

#include "Body.h"

class DriverCourse
{
public:
	static constexpr int RAMP_COURSES = 30;

	explicit DriverCourse(int number);

	static float getGenerationDifficulty(int number);

	bool intersectsBody(const Body& body) const;
	float castRay(sf::Vector2f start, float angle, float length) const;
	float getTargetDist(sf::Vector2f pos, size_t target) const;
	float getTargetDirDiff(sf::Vector2f pos, float rot, size_t target) const;

	int getNumber() const { return number; }
	float getGenerationDifficulty() const { return difficulty; }
	sf::Vector2f getSize() const { return size; }
	float getPathLength() const { return pathLength; }
	float getHalfWidth(size_t index) const { return halfWidths[index]; }
	float getMinRoadWidth() const { return *std::min_element(halfWidths.begin(), halfWidths.end()) * 2.0f; }
	float getMaxRoadWidth() const { return *std::max_element(halfWidths.begin(), halfWidths.end()) * 2.0f; }
	float getTargetRadius() const { return targetRadius; }
	size_t getTargetCount() const { return targets.size(); }
	const sf::Vector2f& getTarget(size_t index) const { return targets[index]; }
	sf::Vector2f getStartPos() const { return path.front(); }
	float getStartRot() const { return startRot; }
	const std::vector<sf::Vector2f>& getPath() const { return path; }
	const std::vector<sf::Vector2f>& getLeftEdge() const { return leftEdge; }
	const std::vector<sf::Vector2f>& getRightEdge() const { return rightEdge; }
	const std::vector<Body>& getWalls() const { return walls; }

private:
	int number = 0;
	float difficulty = 0.0f;
	sf::Vector2f size;
	float pathLength = 0.0f;
	float targetRadius = 0.0f;
	float startRot = 0.0f;
	std::vector<float> halfWidths;
	std::vector<sf::Vector2f> path;
	std::vector<sf::Vector2f> leftEdge;
	std::vector<sf::Vector2f> rightEdge;
	std::vector<sf::Vector2f> targets;
	std::vector<Body> walls;
	std::vector<float> wallRadii;
};

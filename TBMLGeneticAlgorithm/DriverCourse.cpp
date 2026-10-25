#include "stdafx.h"
#include <random>
#include "DriverCourse.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float WALL_THICKNESS = 30.0f;
	constexpr float MARGIN = 160.0f;
	constexpr float MIN_LEG = 240.0f;
	constexpr float MAX_LEG = 340.0f;
	constexpr float MIN_STRAIGHT = 520.0f;
	constexpr float MAX_STRAIGHT = 760.0f;
	constexpr float SAMPLE_SPACING = 40.0f;
	constexpr float CAR_RADIUS = 25.0f;
	constexpr int SEARCH_BUDGET = 4000;

	float pointSegmentDistance(sf::Vector2f point, sf::Vector2f a, sf::Vector2f b)
	{
		const sf::Vector2f segment = b - a;
		const float t = std::clamp((point - a).dot(segment) / segment.lengthSquared(), 0.0f, 1.0f);
		return (point - (a + segment * t)).length();
	}

	struct PathSearch
	{
		std::mt19937& rng;
		sf::Vector2f size;
		size_t legs = 0;
		float maxTurn = 0.0f;
		float straightChance = 0.0f;
		float separation = 0.0f;
		int budget = SEARCH_BUDGET;
		std::vector<sf::Vector2f> path;

		float random(float low, float high) { return low + (high - low) * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng); }

		bool fits(sf::Vector2f from, sf::Vector2f next) const
		{
			if (next.x < MARGIN || next.x > size.x - MARGIN || next.y < MARGIN || next.y > size.y - MARGIN) return false;

			// Clear of every earlier leg except the one this grows out of
			const sf::Vector2f step = next - from;
			for (float d = SAMPLE_SPACING; d <= step.length(); d += SAMPLE_SPACING)
			{
				const sf::Vector2f sample = from + step * (d / step.length());
				for (size_t i = 0; i + 2 < path.size(); i++)
				{
					if (pointSegmentDistance(sample, path[i], path[i + 1]) < separation) return false;
				}
			}
			return true;
		}

		bool extend(float heading)
		{
			if (path.size() > legs) return true;
			if (budget-- <= 0) return false;

			// Mostly corners, with the odd long straight, backing out when a leg leads into a dead end
			for (int tries = 0; tries < 10; tries++)
			{
				float turn = 0.0f;
				float length = 0.0f;
				if (random(0.0f, 1.0f) < straightChance)
				{
					turn = random(-0.12f, 0.12f);
					length = random(MIN_STRAIGHT, MAX_STRAIGHT);
				}
				else
				{
					turn = (random(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f) * random(0.3f * maxTurn, maxTurn);
					length = random(MIN_LEG, MAX_LEG);
				}

				const float candidate = heading + turn;
				const sf::Vector2f from = path.back();
				const sf::Vector2f next = from + sf::Vector2f(std::cos(candidate), std::sin(candidate)) * length;
				if (!this->fits(from, next)) continue;

				path.push_back(next);
				if (this->extend(candidate)) return true;
				path.pop_back();
			}
			return false;
		}
	};

	std::vector<sf::Vector2f> makePath(std::mt19937& rng, sf::Vector2f size, size_t legs, float maxTurn, float straightChance, float separation)
	{
		for (int attempt = 0; attempt < 50; attempt++)
		{
			PathSearch search{ rng, size, legs, maxTurn, straightChance, separation };
			search.path = { { search.random(MARGIN, size.x - MARGIN), search.random(size.y - 260.0f, size.y - MARGIN) } };
			if (search.extend(-PI / 2.0f + search.random(-0.5f, 0.5f))) return search.path;
		}

		// Nothing fitted, so snake back and forth across the arena instead
		std::vector<sf::Vector2f> fallback;
		const float rowSpacing = std::max(separation, 280.0f);
		for (int row = 0; size.y - MARGIN - static_cast<float>(row) * rowSpacing >= MARGIN; row++)
		{
			const float y = size.y - MARGIN - static_cast<float>(row) * rowSpacing;
			const sf::Vector2f left(MARGIN, y);
			const sf::Vector2f right(size.x - MARGIN, y);
			fallback.push_back(row % 2 == 0 ? left : right);
			fallback.push_back(row % 2 == 0 ? right : left);
		}
		fallback.resize(std::min(fallback.size(), legs + 1));
		return fallback;
	}
}

DriverCourse::DriverCourse(int number)
	: number(number)
{
	std::mt19937 rng(static_cast<unsigned int>(number) * 7919u + 17u);
	auto random = [&](float low, float high) { return low + (high - low) * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng); };

	// Later courses are much longer, with a narrower road that varies more, sharper and more frequent bends, and smaller targets
	difficulty = getGenerationDifficulty(number);
	size = { std::lerp(1400.0f, 2800.0f, difficulty), std::lerp(1000.0f, 1800.0f, difficulty) };
	targetRadius = std::lerp(42.0f, 30.0f, difficulty);
	const float baseHalfWidth = std::lerp(120.0f, 80.0f, difficulty);
	const float variation = std::lerp(0.1f, 0.35f, difficulty);
	const float maxTurn = std::lerp(0.45f, 1.0f, difficulty);
	const float straightChance = std::lerp(0.15f, 0.3f, difficulty);
	const size_t legs = static_cast<size_t>(std::lerp(3.0f, 14.0f, difficulty) + 0.5f);
	const float separation = 2.0f * baseHalfWidth * (1.0f + variation) + WALL_THICKNESS + 30.0f;

	path = makePath(rng, size, legs, maxTurn, straightChance, separation);
	const size_t count = path.size();

	std::vector<sf::Vector2f> directions(count - 1);
	for (size_t i = 0; i + 1 < count; i++)
	{
		directions[i] = (path[i + 1] - path[i]).normalized();
		pathLength += (path[i + 1] - path[i]).length();
	}

	// Edges of the road, mitred at each waypoint so the corners stay sealed, with the road widening and narrowing between waypoints
	for (size_t i = 0; i < count; i++)
	{
		halfWidths.push_back(baseHalfWidth * (1.0f + random(-variation, variation)));

		const sf::Vector2f before = directions[i == 0 ? 0 : i - 1];
		const sf::Vector2f after = directions[i + 1 < count ? i : i - 1];
		const sf::Vector2f normalBefore(-before.y, before.x);
		const sf::Vector2f normalAfter(-after.y, after.x);
		const sf::Vector2f miter = (normalBefore + normalAfter).normalized();
		const sf::Vector2f offset = miter * (halfWidths[i] / std::max(0.5f, miter.dot(normalAfter)));
		leftEdge.push_back(path[i] + offset);
		rightEdge.push_back(path[i] - offset);
	}

	// A wall along each edge of each leg, with its inner face on the edge
	auto addWall = [&](sf::Vector2f from, sf::Vector2f to, float side)
	{
		const sf::Vector2f delta = to - from;
		const sf::Vector2f outward = sf::Vector2f(-delta.y, delta.x).normalized() * side;
		const sf::Vector2f centre = (from + to) / 2.0f + outward * (WALL_THICKNESS / 2.0f);
		walls.push_back(Body(centre, { delta.length() + WALL_THICKNESS, WALL_THICKNESS }, std::atan2(delta.y, delta.x)));
	};
	for (size_t i = 0; i + 1 < count; i++)
	{
		addWall(leftEdge[i], leftEdge[i + 1], 1.0f);
		addWall(rightEdge[i], rightEdge[i + 1], -1.0f);
	}

	// Caps closing off both ends of the road
	const sf::Vector2f startDir = directions.front();
	const sf::Vector2f endDir = directions.back();
	walls.push_back(Body(path.front() - startDir * 70.0f, { WALL_THICKNESS, 2.0f * halfWidths.front() + 2.0f * WALL_THICKNESS }, std::atan2(startDir.y, startDir.x)));
	walls.push_back(Body(path.back() + endDir * (targetRadius + WALL_THICKNESS), { WALL_THICKNESS, 2.0f * halfWidths.back() + 2.0f * WALL_THICKNESS }, std::atan2(endDir.y, endDir.x)));
	for (Body& wall : walls)
	{
		wall.recalculateVertices();
		wallRadii.push_back(wall.size.length() / 2.0f);
	}

	// Cars start facing roughly down the road, a little off later on
	startRot = std::atan2(startDir.y, startDir.x) + random(-0.3f, 0.3f) * difficulty;
	targets.assign(path.begin() + 1, path.end());
}

float DriverCourse::getGenerationDifficulty(int number)
{
	return std::min(1.0f, static_cast<float>(number - 1) / static_cast<float>(RAMP_COURSES));
}

bool DriverCourse::intersectsBody(const Body& body) const
{
	for (size_t i = 0; i < walls.size(); i++)
	{
		if ((walls[i].pos - body.pos).length() > wallRadii[i] + CAR_RADIUS) continue;
		if (body.intersectBody(walls[i])) return true;
	}
	return false;
}

float DriverCourse::castRay(sf::Vector2f start, float angle, float length) const
{
	// Fraction of the ray's length before it hits a wall, 1 if it hits nothing
	const sf::Vector2f end = start + sf::Vector2f(std::cos(angle), std::sin(angle)) * length;
	float nearest = 1.0f;
	for (size_t i = 0; i < walls.size(); i++)
	{
		if (pointSegmentDistance(walls[i].pos, start, end) > wallRadii[i]) continue;
		nearest = std::min(nearest, walls[i].raycast(start, end));
	}
	return nearest;
}

float DriverCourse::getTargetDist(sf::Vector2f pos, size_t target) const
{
	return (targets[target] - pos).length() - targetRadius;
}

float DriverCourse::getTargetDirDiff(sf::Vector2f pos, float rot, size_t target) const
{
	const sf::Vector2f toTarget = targets[target] - pos;
	return std::remainder(std::atan2(toTarget.y, toTarget.x) - rot, 2.0f * PI);
}

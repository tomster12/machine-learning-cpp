#include "stdafx.h"
#include "WalkerAgent.h"
#include "Utility.h"
#include "UITheme.h"

namespace
{
	constexpr float PI = 3.14159265359f;
	constexpr float GRAVITY = 1100.0f;
	constexpr float AIR_DAMPING = 0.999f;
	constexpr float MAX_SPEED = 800.0f;
	constexpr float TORSO_LENGTH = 44.0f;
	constexpr float THIGH_LENGTH = 36.0f;
	constexpr float SHIN_LENGTH = 36.0f;
	constexpr float FOOT_RADIUS = 3.0f;
	constexpr float HIP_RANGE = 0.9f;
	constexpr float KNEE_MAX = 1.7f;
	constexpr float DRIVE_RATE = 0.05f;
	constexpr float JOINT_STIFFNESS = 0.5f;
	constexpr int ITERATIONS = 6;
	constexpr float CLOCK_PERIOD = 0.9f;
	constexpr float STAND_HEIGHT = 80.0f;
	constexpr float FALL_HIP_HEIGHT = 30.0f;
	constexpr float FALL_NECK_HEIGHT = 45.0f;
	constexpr float FALL_TILT = 1.3f;
	constexpr float UPRIGHT_TILT = 0.6f;
	constexpr float UPRIGHT_HEIGHT = 55.0f;
	constexpr float HEAD_RADIUS = 10.0f;

	float wrapAngle(float angle) { return std::remainder(angle, 2.0f * PI); }
	float cross(sf::Vector2f a, sf::Vector2f b) { return a.x * b.y - a.y * b.x; }
	float dot(sf::Vector2f a, sf::Vector2f b) { return a.x * b.x + a.y * b.y; }
	sf::Color shade(sf::Color colour, float factor) { return sf::Color(static_cast<std::uint8_t>(colour.r * factor), static_cast<std::uint8_t>(colour.g * factor), static_cast<std::uint8_t>(colour.b * factor), colour.a); }
}

WalkerAgent::WalkerAgent(WalkerAgent::GenomeCPtr&& genome, std::shared_ptr<const WalkerCourse> course)
	: Agent(std::move(genome)), course(std::move(course)), network(this->genome->copyNetwork()), netInput({ 1, INPUT_COUNT }, 0.0f)
{
	// Stand straight up on the start line, heavy hips and shoulders over light legs
	const float startX = WalkerCourse::START_X;
	const float hipY = this->course->getHeight(startX) - THIGH_LENGTH - SHIN_LENGTH - FOOT_RADIUS;
	auto place = [&](size_t index, float offsetY, float mass, float radius, float friction)
	{
		const sf::Vector2f pos(startX, hipY + offsetY);
		points[index] = { pos, pos, 1.0f / mass, radius, friction, false };
	};
	place(HIP, 0.0f, 3.0f, 5.0f, 0.4f);
	place(NECK, -TORSO_LENGTH, 4.0f, 5.0f, 0.4f);
	place(KNEE_L, THIGH_LENGTH, 1.0f, 3.5f, 0.5f);
	place(FOOT_L, THIGH_LENGTH + SHIN_LENGTH, 0.6f, FOOT_RADIUS, 0.9f);
	place(KNEE_R, THIGH_LENGTH, 1.0f, 3.5f, 0.5f);
	place(FOOT_R, THIGH_LENGTH + SHIN_LENGTH, 0.6f, FOOT_RADIUS, 0.9f);

	// Legs start straight, the knee drive sits at its lower end
	drive = { 0.0f, -1.0f, 0.0f, -1.0f };
	bestX = startX;

	this->sense();
	this->calculateFitness();
}

bool WalkerAgent::evaluate()
{
	if (isFinished) return true;

	// Think, then set the joint drives towards what the network asked for
	this->sense();
	this->act(*network.propogatePtr(&netInput));

	this->simulate();
	step++;
	this->checkOutcome();

	// Keep fitness current so the elite can be picked mid generation
	this->calculateFitness();
	return isFinished;
}

void WalkerAgent::sense()
{
	// Everything is scaled to roughly -1 to 1 so no input drowns out the others
	const sf::Vector2f velocity = this->getHipVelocity();
	netInput.at(0, 0) = tilt / (PI / 2.0f);
	netInput.at(0, 1) = spin / 6.0f;
	netInput.at(0, 2) = this->getHipHeight() / STAND_HEIGHT;
	netInput.at(0, 3) = velocity.x / 200.0f;
	netInput.at(0, 4) = velocity.y / 200.0f;
	for (size_t i = 0; i < JOINT_COUNT; i++) netInput.at(0, 5 + i) = drive[i];
	netInput.at(0, 9) = footContact[0] ? 1.0f : 0.0f;
	netInput.at(0, 10) = footContact[1] ? 1.0f : 0.0f;

	// A clock gives the network a rhythm to build a gait on
	const float phase = 2.0f * PI * static_cast<float>(step) * TIME_STEP / CLOCK_PERIOD;
	netInput.at(0, 11) = std::sin(phase);
	netInput.at(0, 12) = std::cos(phase);

	// Slope of the ground a few strides ahead
	const float hipX = points[HIP].pos.x;
	const float ahead[3] = { 30.0f, 70.0f, 120.0f };
	for (size_t i = 0; i < 3; i++) netInput.at(0, 13 + i) = (course->getHeight(hipX + ahead[i]) - course->getHeight(hipX)) / 60.0f;
}

void WalkerAgent::act(const tbml::Tensor& output)
{
	// Joints slew towards their target so the legs can't snap between extremes
	for (size_t i = 0; i < JOINT_COUNT; i++)
	{
		const float target = std::clamp(output.at(0, i), -1.0f, 1.0f);
		drive[i] += std::clamp(target - drive[i], -DRIVE_RATE, DRIVE_RATE);
	}
}

float WalkerAgent::getJointAngle(size_t joint) const
{
	// Hips swing either way, knees only bend one way and ease in from straight
	if (joint % 2 == 0) return drive[joint] * HIP_RANGE;
	const float bend = drive[joint] * 0.5f + 0.5f;
	return KNEE_MAX * bend * bend;
}

void WalkerAgent::simulate()
{
	const float dt = TIME_STEP / static_cast<float>(SUBSTEPS);
	for (int substep = 0; substep < SUBSTEPS; substep++)
	{
		this->integrate(dt);
		for (int i = 0; i < ITERATIONS; i++)
		{
			this->solveRod(HIP, NECK, TORSO_LENGTH);
			this->solveRod(HIP, KNEE_L, THIGH_LENGTH);
			this->solveRod(KNEE_L, FOOT_L, SHIN_LENGTH);
			this->solveRod(HIP, KNEE_R, THIGH_LENGTH);
			this->solveRod(KNEE_R, FOOT_R, SHIN_LENGTH);

			// Hip angle is measured from the torso, knee angle from the thigh
			this->solveJoint(NECK, HIP, KNEE_L, PI - this->getJointAngle(0));
			this->solveJoint(HIP, KNEE_L, FOOT_L, this->getJointAngle(1) - PI);
			this->solveJoint(NECK, HIP, KNEE_R, PI - this->getJointAngle(2));
			this->solveJoint(HIP, KNEE_R, FOOT_R, this->getJointAngle(3) - PI);

			this->collide();
		}
		this->applyFriction();
	}

	footContact[0] = points[FOOT_L].touching;
	footContact[1] = points[FOOT_R].touching;

	// Torso lean and how fast it is changing
	const sf::Vector2f torso = points[NECK].pos - points[HIP].pos;
	const float newTilt = std::atan2(torso.x, -torso.y);
	spin = wrapAngle(newTilt - tilt) / TIME_STEP;
	tilt = newTilt;
}

void WalkerAgent::integrate(float dt)
{
	for (Point& point : points)
	{
		sf::Vector2f velocity = (point.pos - point.prev) * AIR_DAMPING;
		const float speed = velocity.length();
		if (speed > MAX_SPEED * dt) velocity *= MAX_SPEED * dt / speed;
		point.prev = point.pos;
		point.pos += velocity + sf::Vector2f(0.0f, GRAVITY) * (dt * dt);
		point.touching = false;
	}
}

void WalkerAgent::solveRod(size_t a, size_t b, float length)
{
	Point& pa = points[a];
	Point& pb = points[b];
	const sf::Vector2f delta = pb.pos - pa.pos;
	const float distance = delta.length();
	const float weight = pa.invMass + pb.invMass;
	if (distance < 0.0001f || weight <= 0.0f) return;

	const sf::Vector2f correction = delta * ((distance - length) / (distance * weight));
	pa.pos += correction * pa.invMass;
	pb.pos -= correction * pb.invMass;
}

void WalkerAgent::solveJoint(size_t a, size_t pivot, size_t b, float target)
{
	Point& pa = points[a];
	Point& pp = points[pivot];
	Point& pb = points[b];
	const sf::Vector2f u = pa.pos - pp.pos;
	const sf::Vector2f v = pb.pos - pp.pos;
	const float uu = dot(u, u);
	const float vv = dot(v, v);
	if (uu < 0.0001f || vv < 0.0001f) return;

	// Signed angle from u to v and the way each end must move to change it
	const float error = wrapAngle(std::atan2(cross(u, v), dot(u, v)) - target);
	const sf::Vector2f gradA = sf::Vector2f(u.y, -u.x) / uu;
	const sf::Vector2f gradB = sf::Vector2f(-v.y, v.x) / vv;
	const sf::Vector2f gradP = -(gradA + gradB);
	const float weight = pa.invMass * dot(gradA, gradA) + pb.invMass * dot(gradB, gradB) + pp.invMass * dot(gradP, gradP);
	if (weight <= 0.0f) return;

	const float lambda = -error * JOINT_STIFFNESS / weight;
	pa.pos += gradA * (lambda * pa.invMass);
	pb.pos += gradB * (lambda * pb.invMass);
	pp.pos += gradP * (lambda * pp.invMass);
}

void WalkerAgent::collide()
{
	for (Point& point : points)
	{
		const float floor = course->getHeight(point.pos.x) - point.radius;
		if (point.pos.y > floor)
		{
			point.pos.y = floor;
			point.touching = true;
		}
	}
}

void WalkerAgent::applyFriction()
{
	// Grip scales sideways speed down and kills any bounce
	for (Point& point : points)
	{
		if (!point.touching) continue;

		point.prev.x = point.pos.x - (point.pos.x - point.prev.x) * (1.0f - point.friction);
		if (point.pos.y > point.prev.y) point.prev.y = point.pos.y;
	}
}

float WalkerAgent::getHipHeight() const
{
	return course->getHeight(points[HIP].pos.x) - points[HIP].pos.y;
}

void WalkerAgent::checkOutcome()
{
	bestX = std::max(bestX, points[HIP].pos.x);

	const float hipHeight = this->getHipHeight();
	const float neckHeight = course->getHeight(points[NECK].pos.x) - points[NECK].pos.y;
	if (std::abs(tilt) < UPRIGHT_TILT && hipHeight > UPRIGHT_HEIGHT) uprightSteps++;

	if (points[HIP].pos.x >= WalkerCourse::GOAL_X) outcome = WalkerOutcome::FINISHED;
	else if (hipHeight < FALL_HIP_HEIGHT || neckHeight < FALL_NECK_HEIGHT || std::abs(tilt) > FALL_TILT) outcome = WalkerOutcome::FELL;
	else if (step >= MAX_STEPS) outcome = WalkerOutcome::TIMED_OUT;

	isFinished = outcome != WalkerOutcome::WALKING;
}

void WalkerAgent::calculateFitness()
{
	// Distance covered, discounted for crawling and tumbling instead of walking upright
	const float progress = std::clamp((bestX - WalkerCourse::START_X) / (WalkerCourse::GOAL_X - WalkerCourse::START_X), 0.0f, 1.0f);
	const float upright = step > 0 ? static_cast<float>(uprightSteps) / static_cast<float>(step) : 1.0f;

	float score = progress * (0.6f + 0.4f * upright);
	if (outcome == WalkerOutcome::FINISHED) score = 1.2f + 0.5f * (1.0f - static_cast<float>(step) / static_cast<float>(MAX_STEPS));

	// Square fitness to sharpen selection
	fitness = score * score;
}

sf::Vector2f WalkerAgent::getHeadPosition() const
{
	const sf::Vector2f up = points[NECK].pos - points[HIP].pos;
	const float length = up.length();
	if (length < 0.0001f) return points[NECK].pos;
	return points[NECK].pos + up / length * (HEAD_RADIUS - 2.0f);
}

sf::Color WalkerAgent::stateColour(std::uint8_t alpha) const
{
	sf::Color base = theme::WALKER_WALKING;
	if (outcome == WalkerOutcome::FELL) base = theme::WALKER_FELL;
	else if (outcome == WalkerOutcome::TIMED_OUT) base = theme::WALKER_TIMED_OUT;
	else if (outcome == WalkerOutcome::FINISHED) base = theme::WALKER_FINISHED;
	return sf::Color(base.r, base.g, base.b, alpha);
}

void WalkerAgent::appendPackVertices(sf::VertexArray& vertices) const
{
	// Colour shows the outcome, opacity shows role: walkers still going are clearer than finished ones
	const sf::Color colour = stateColour(isFinished ? 60 : 120);
	theme::appendLine(vertices, points[HIP].pos, points[NECK].pos, 2.5f, colour);
	theme::appendLine(vertices, points[HIP].pos, points[KNEE_L].pos, 2.0f, colour);
	theme::appendLine(vertices, points[KNEE_L].pos, points[FOOT_L].pos, 2.0f, colour);
	theme::appendLine(vertices, points[HIP].pos, points[KNEE_R].pos, 2.0f, colour);
	theme::appendLine(vertices, points[KNEE_R].pos, points[FOOT_R].pos, 2.0f, colour);
}

void WalkerAgent::renderElite(sf::RenderWindow* window) const
{
	const sf::Color colour = stateColour(255);
	const sf::Color rear = shade(colour, 0.55f);
	const sf::Vector2f head = this->getHeadPosition();

	// Soft halo so the elite reads against the pack
	sf::CircleShape halo(70.0f);
	halo.setOrigin({ 70.0f, 70.0f });
	halo.setPosition((points[HIP].pos + points[NECK].pos) / 2.0f);
	halo.setFillColor(sf::Color(colour.r, colour.g, colour.b, 18));
	window->draw(halo);

	// Far leg first, then the torso, then the near leg on top
	sf::VertexArray body(sf::PrimitiveType::Triangles);
	theme::appendLine(body, points[HIP].pos, points[KNEE_R].pos, 7.0f, rear);
	theme::appendLine(body, points[KNEE_R].pos, points[FOOT_R].pos, 5.5f, rear);
	theme::appendDisc(body, points[KNEE_R].pos, 3.5f, rear);
	theme::appendDisc(body, points[FOOT_R].pos, 4.5f, rear);
	theme::appendLine(body, points[HIP].pos, points[NECK].pos, 10.0f, shade(colour, 0.85f));
	theme::appendDisc(body, points[HIP].pos, 6.0f, colour);
	theme::appendDisc(body, points[NECK].pos, 5.5f, colour);
	theme::appendLine(body, points[HIP].pos, points[KNEE_L].pos, 7.0f, colour);
	theme::appendLine(body, points[KNEE_L].pos, points[FOOT_L].pos, 5.5f, colour);
	theme::appendDisc(body, points[KNEE_L].pos, 3.5f, colour);
	theme::appendDisc(body, points[FOOT_L].pos, 4.5f, colour);
	theme::appendDisc(body, head, HEAD_RADIUS, colour, 16);
	window->draw(body);

	// Fitness readout on a small plate above the head, kept upright
	const std::string label = global::formatFloat(fitness, 2);
	const float plateWidth = theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
	const sf::Vector2f centre(head.x, head.y - 28.0f);
	theme::drawRect(window, { centre.x - plateWidth / 2.0f, centre.y - 9.0f }, { plateWidth, 18.0f }, theme::PANEL_FILL);
	theme::drawText(window, label, centre, theme::TEXT_SMALL, theme::TEXT_PRIMARY, theme::Align::CENTER);
}

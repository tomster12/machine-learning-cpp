#pragma once

class DogfightShip
{
public:
	static constexpr float RADIUS = 6.3f;
	static constexpr float THRUST = 340.0f;
	static constexpr float BOOST_THRUST = 1500.0f;
	static constexpr float MAX_SPEED = 360.0f;
	static constexpr float BOOST_SPEED = 640.0f;
	static constexpr float TURN_SPEED = 4.2f;
	static constexpr float DRAG = 0.25f;
	static constexpr float BRAKE_DRAG = 3.0f;
	static constexpr float BULLET_SPEED = 400.0f;
	static constexpr float BULLET_LIFE = 1.7f;
	static constexpr float BULLET_RADIUS = 6.0f;
	static constexpr float GUN_COOLDOWN = 0.5f;
	static constexpr float BOOST_TIME = 0.25f;
	static constexpr float BOOST_COOLDOWN = 4.0f;
	static constexpr float SHIELD_TIME = 0.8f;
	static constexpr float SHIELD_COOLDOWN = 10.0f;
	static constexpr float MAX_HEALTH = 100.0f;
	static constexpr float HIT_GRACE = 0.35f;

	struct Controls
	{
		float thrust = 0.0f;
		float turn = 0.0f;
		bool brake = false;
		bool shoot = false;
		bool boost = false;
		bool shield = false;
	};

	DogfightShip() = default;
	DogfightShip(sf::Vector2f pos, float rot);

	void control(const Controls& newControls, bool boostEnabled, bool shieldEnabled);
	void step(float dt);
	bool takeShot();
	float takeDamage(float amount);
	void nudge(sf::Vector2f offset, sf::Vector2f velocityChange);
	void appendGhost(sf::VertexArray& vertices, sf::Color colour) const;
	void render(sf::RenderWindow* window, sf::Color colour, float time) const;

	bool consumeBoostStart() { return std::exchange(boostStarted, false); }
	bool consumeShieldStart() { return std::exchange(shieldStarted, false); }
	bool isAlive() const { return health > 0.0f; }
	bool isBoosting() const { return boostTimer > 0.0f; }
	bool isShielded() const { return shieldTimer > 0.0f; }
	sf::Vector2f getPosition() const { return pos; }
	sf::Vector2f getVelocity() const { return vel; }
	sf::Vector2f getHeading() const { return { std::cos(rot), std::sin(rot) }; }
	float getRotation() const { return rot; }
	float getAngularVelocity() const { return angVel; }
	float getThrottle() const { return controls.thrust; }
	float getHealth() const { return health; }
	float getHealthFraction() const { return std::max(0.0f, health) / MAX_HEALTH; }
	float getGunReadiness() const { return 1.0f - std::clamp(gunTimer / GUN_COOLDOWN, 0.0f, 1.0f); }
	float getBoostReadiness() const { return 1.0f - std::clamp(boostCooldown / BOOST_COOLDOWN, 0.0f, 1.0f); }
	float getShieldReadiness() const { return 1.0f - std::clamp(shieldCooldown / SHIELD_COOLDOWN, 0.0f, 1.0f); }
	float getShieldRemaining() const { return std::clamp(shieldTimer / SHIELD_TIME, 0.0f, 1.0f); }

private:
	void renderAt(sf::RenderWindow* window, sf::Vector2f centre, sf::Color colour, float time) const;

	sf::Vector2f pos;
	sf::Vector2f vel;
	float rot = 0.0f;
	float angVel = 0.0f;
	float health = MAX_HEALTH;
	float gunTimer = 0.0f;
	float boostTimer = 0.0f;
	float boostCooldown = 0.0f;
	float shieldTimer = 0.0f;
	float shieldCooldown = 0.0f;
	float graceTimer = 0.0f;
	bool boostStarted = false;
	bool shieldStarted = false;
	Controls controls;
};

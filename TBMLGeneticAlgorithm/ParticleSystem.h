#pragma once

class ParticleSystem
{
public:
	void update(float dt);
	void emit(sf::Vector2f pos, sf::Vector2f vel, sf::Color colour, float life, float size, float gravity = 0.0f);
	void ripple(sf::Vector2f pos, float radius, sf::Color colour);
	void render(sf::RenderWindow* window) const;

private:
	struct Particle
	{
		sf::Vector2f pos;
		sf::Vector2f vel;
		sf::Color colour;
		float life = 0.0f;
		float maxLife = 1.0f;
		float size = 2.0f;
		float gravity = 0.0f;
	};

	struct Ripple
	{
		sf::Vector2f pos;
		sf::Color colour;
		float radius = 0.0f;
		float age = 0.0f;
	};

	std::vector<Particle> particles;
	std::vector<Ripple> ripples;
};

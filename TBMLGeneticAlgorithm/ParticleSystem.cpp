#include "stdafx.h"
#include "ParticleSystem.h"
#include "UITheme.h"

namespace
{
	constexpr size_t MAX_PARTICLES = 1000;
	constexpr float RIPPLE_LIFE = 0.9f;
	constexpr float RIPPLE_GROWTH = 70.0f;
}

void ParticleSystem::update(float dt)
{
	// Age, move and drop the dead
	for (Particle& particle : particles)
	{
		particle.life -= dt;
		particle.vel.y += particle.gravity * dt;
		particle.pos += particle.vel * dt;
	}
	std::erase_if(particles, [](const Particle& particle) { return particle.life <= 0.0f; });

	for (Ripple& ripple : ripples) ripple.age += dt;
	std::erase_if(ripples, [](const Ripple& ripple) { return ripple.age >= RIPPLE_LIFE; });
}

void ParticleSystem::emit(sf::Vector2f pos, sf::Vector2f vel, sf::Color colour, float life, float size, float gravity)
{
	if (particles.size() >= MAX_PARTICLES) return;

	particles.push_back({ pos, vel, colour, life, life, size, gravity });
}

void ParticleSystem::ripple(sf::Vector2f pos, float radius, sf::Color colour)
{
	ripples.push_back({ pos, colour, radius, 0.0f });
}

void ParticleSystem::render(sf::RenderWindow* window) const
{
	sf::VertexArray vertices(sf::PrimitiveType::Triangles);
	for (const Particle& particle : particles)
	{
		const float t = particle.life / particle.maxLife;
		const sf::Color colour(particle.colour.r, particle.colour.g, particle.colour.b, theme::toAlpha(230.0f * t));
		theme::appendDisc(vertices, particle.pos, particle.size * (0.4f + 0.6f * t) * 0.5f, colour, 6);
	}
	window->draw(vertices);

	for (const Ripple& ripple : ripples)
	{
		const float t = ripple.age / RIPPLE_LIFE;
		theme::drawRing(window, ripple.pos, ripple.radius + RIPPLE_GROWTH * t, 2.0f, sf::Color(ripple.colour.r, ripple.colour.g, ripple.colour.b, theme::toAlpha(220.0f * (1.0f - t))));
	}
}

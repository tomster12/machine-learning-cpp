#pragma once

#include "UITheme.h"

enum class Icon { PLAY, PAUSE, RUN_TO_END, NEXT, LOOP, FAST, MINUS, PLUS };

namespace icons
{
	inline void polygon(sf::VertexArray& out, std::initializer_list<sf::Vector2f> points, sf::Color colour)
	{
		// Triangle fan from the first point, so every shape lands in one vertex array
		const sf::Vector2f* p = points.begin();
		for (size_t i = 1; i + 1 < points.size(); i++)
		{
			out.append(sf::Vertex{ p[0], colour });
			out.append(sf::Vertex{ p[i], colour });
			out.append(sf::Vertex{ p[i + 1], colour });
		}
	}

	inline void rect(sf::VertexArray& out, sf::Vector2f pos, sf::Vector2f size, sf::Color colour)
	{
		polygon(out, { pos, { pos.x + size.x, pos.y }, pos + size, { pos.x, pos.y + size.y } }, colour);
	}

	inline void line(sf::VertexArray& out, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color colour)
	{
		const sf::Vector2f normal = sf::Vector2f(-(b - a).y, (b - a).x).normalized() * (thickness / 2.0f);
		polygon(out, { a + normal, b + normal, b - normal, a - normal }, colour);
	}

	inline void draw(sf::RenderWindow* window, Icon icon, sf::Vector2f c, float s, sf::Color colour)
	{
		// Reused between calls so drawing an icon is one draw call and no allocation
		static sf::VertexArray out(sf::PrimitiveType::Triangles);
		out.clear();

		// c is the centre and s the overall extent in pixels
		switch (icon)
		{
		case Icon::PLAY:
			polygon(out, { { c.x - 0.35f * s, c.y - 0.5f * s }, { c.x - 0.35f * s, c.y + 0.5f * s }, { c.x + 0.55f * s, c.y } }, colour);
			break;

		case Icon::PAUSE:
			rect(out, { c.x - 0.4f * s, c.y - 0.5f * s }, { 0.28f * s, s }, colour);
			rect(out, { c.x + 0.12f * s, c.y - 0.5f * s }, { 0.28f * s, s }, colour);
			break;

		case Icon::RUN_TO_END:
			polygon(out, { { c.x - 0.5f * s, c.y - 0.45f * s }, { c.x - 0.5f * s, c.y + 0.45f * s }, { c.x + 0.25f * s, c.y } }, colour);
			rect(out, { c.x + 0.35f * s, c.y - 0.45f * s }, { 0.18f * s, 0.9f * s }, colour);
			break;

		case Icon::NEXT:
			line(out, { c.x - 0.5f * s, c.y }, { c.x + 0.15f * s, c.y }, 2.0f, colour);
			polygon(out, { { c.x + 0.05f * s, c.y - 0.4f * s }, { c.x + 0.05f * s, c.y + 0.4f * s }, { c.x + 0.55f * s, c.y } }, colour);
			break;

		case Icon::LOOP:
		{
			// Open circle with an arrowhead on the end
			const float radius = 0.42f * s;
			const float thickness = std::max(1.0f, 0.17f * s);
			const float start = 1.0f;
			const float end = 5.6f;
			const int segments = 14;
			sf::Vector2f previous;
			for (int i = 0; i <= segments; i++)
			{
				const float angle = start + (end - start) * static_cast<float>(i) / static_cast<float>(segments);
				const sf::Vector2f point(c.x + radius * std::cos(angle), c.y + radius * std::sin(angle));
				if (i > 0) line(out, previous, point, thickness, colour);
				previous = point;
			}
			const sf::Vector2f radial(std::cos(end), std::sin(end));
			const sf::Vector2f tangent(-std::sin(end), std::cos(end));
			polygon(out, { previous + radial * 0.3f * s, previous - radial * 0.3f * s, previous + tangent * 0.45f * s }, colour);
			break;
		}

		case Icon::FAST:
			polygon(out, { { c.x - 0.55f * s, c.y - 0.45f * s }, { c.x - 0.55f * s, c.y + 0.45f * s }, { c.x - 0.05f * s, c.y } }, colour);
			polygon(out, { { c.x - 0.05f * s, c.y - 0.45f * s }, { c.x - 0.05f * s, c.y + 0.45f * s }, { c.x + 0.45f * s, c.y } }, colour);
			break;

		case Icon::PLUS:
			rect(out, { c.x - 1.0f, c.y - 0.4f * s }, { 2.0f, 0.8f * s }, colour);
			[[fallthrough]];
		case Icon::MINUS:
			rect(out, { c.x - 0.4f * s, c.y - 1.0f }, { 0.8f * s, 2.0f }, colour);
			break;
		}

		window->draw(out);
	}
}

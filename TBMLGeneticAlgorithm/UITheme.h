#pragma once

namespace theme
{
	inline const sf::Color PANEL_FILL(16, 17, 19, 218);
	inline const sf::Color PANEL_BORDER(255, 255, 255, 22);
	inline const sf::Color TILE_FILL(255, 255, 255, 14);
	inline const sf::Color TILE_HOVER(255, 255, 255, 34);
	inline const sf::Color TILE_ACTIVE(255, 255, 255, 46);
	inline const sf::Color TILE_ACTIVE_HOVER(255, 255, 255, 66);
	inline const sf::Color HAIRLINE(255, 255, 255, 26);
	inline const sf::Color TEXT_PRIMARY(234, 236, 238);
	inline const sf::Color TEXT_SECONDARY(140, 145, 152);
	inline const sf::Color NEGATIVE(222, 104, 100);
	inline const sf::Color WARNING(230, 188, 90);
	inline const sf::Color INFO(112, 184, 240);
	inline const sf::Color ELITE(120, 206, 150);

	inline const sf::Color CAR_ALIVE(244, 246, 248);
	inline const sf::Color CAR_CRASHED = NEGATIVE;
	inline const sf::Color CAR_TIMED_OUT = WARNING;
	inline const sf::Color CAR_COMPLETED = INFO;
	inline const sf::Color SHIP_FLYING = CAR_ALIVE;
	inline const sf::Color SHIP_LANDED = INFO;
	inline const sf::Color SHIP_CRASHED = NEGATIVE;
	inline const sf::Color SHIP_LOST = WARNING;
	inline const sf::Color WALKER_WALKING = CAR_ALIVE;
	inline const sf::Color WALKER_FELL = NEGATIVE;
	inline const sf::Color WALKER_TIMED_OUT = WARNING;
	inline const sf::Color WALKER_FINISHED = INFO;
	inline const sf::Color CART_BALANCING = CAR_ALIVE;
	inline const sf::Color CART_FELL = NEGATIVE;
	inline const sf::Color CART_OFF_TRACK = WARNING;
	inline const sf::Color CART_SURVIVED = INFO;

	constexpr unsigned int TEXT_HEADER = 14;
	constexpr unsigned int TEXT_BODY = 13;
	constexpr unsigned int TEXT_SMALL = 11;

	constexpr float MARGIN = 8.0f;
	constexpr float PADDING = 10.0f;
	constexpr float GAP = 8.0f;
	constexpr float ROW_HEIGHT = 20.0f;
	constexpr float PANEL_WIDTH = 208.0f;
	constexpr float HEADER_HEIGHT = 28.0f;
	constexpr float HEADER_INSET = 4.0f;
	constexpr float HEADER_TEXT_Y = 12.0f;

	enum class Align { LEFT, CENTER, RIGHT };

	inline void drawText(sf::RenderWindow* window, const std::string& str, sf::Vector2f anchor, unsigned int size, sf::Color colour, Align align = Align::LEFT, bool bold = false)
	{
		// Anchor is the horizontal edge / centre given by align and the vertical middle of the line box, so rows share a baseline
		sf::Text text(global::font, str, size);
		text.setFillColor(colour);
		if (bold) text.setStyle(sf::Text::Bold);

		const sf::FloatRect bounds = text.getLocalBounds();
		float originX = bounds.position.x;
		if (align == Align::CENTER) originX += bounds.size.x / 2.0f;
		else if (align == Align::RIGHT) originX += bounds.size.x;

		text.setOrigin({ originX, global::font.getLineSpacing(size) / 2.0f });
		text.setPosition(anchor);
		window->draw(text);
	}

	inline sf::View screenView(const sf::RenderWindow* window)
	{
		return sf::View(sf::FloatRect({ 0.0f, 0.0f }, sf::Vector2f(window->getSize())));
	}

	inline float measureText(const std::string& str, unsigned int size)
	{
		return sf::Text(global::font, str, size).getLocalBounds().size.x;
	}

	inline void drawRect(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size, sf::Color colour)
	{
		sf::RectangleShape rect(size);
		rect.setPosition(pos);
		rect.setFillColor(colour);
		window->draw(rect);
	}

	inline std::uint8_t toAlpha(float value) { return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 255.0f)); }

	inline void drawRing(sf::RenderWindow* window, sf::Vector2f centre, float radius, float thickness, sf::Color outline, sf::Color fill = sf::Color::Transparent)
	{
		sf::CircleShape ring(radius, 48);
		ring.setOrigin({ radius, radius });
		ring.setPosition(centre);
		ring.setFillColor(fill);
		ring.setOutlineColor(outline);
		ring.setOutlineThickness(thickness);
		window->draw(ring);
	}

	inline void appendQuad(sf::VertexArray& vertices, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d, sf::Color colour)
	{
		vertices.append({ a, colour });
		vertices.append({ b, colour });
		vertices.append({ c, colour });
		vertices.append({ a, colour });
		vertices.append({ c, colour });
		vertices.append({ d, colour });
	}

	inline void appendLine(sf::VertexArray& vertices, sf::Vector2f from, sf::Vector2f to, float width, sf::Color colour)
	{
		const sf::Vector2f delta = to - from;
		const float length = delta.length();
		if (length < 0.001f) return;

		const sf::Vector2f offset = sf::Vector2f(-delta.y, delta.x) / length * (width / 2.0f);
		appendQuad(vertices, from + offset, to + offset, to - offset, from - offset, colour);
	}

	inline void appendDisc(sf::VertexArray& vertices, sf::Vector2f centre, float radius, sf::Color colour, int segments = 10)
	{
		const float step = 6.28318530718f / static_cast<float>(segments);
		for (int i = 0; i < segments; i++)
		{
			const float a = step * static_cast<float>(i);
			const float b = step * static_cast<float>(i + 1);
			vertices.append({ centre, colour });
			vertices.append({ centre + sf::Vector2f(std::cos(a), std::sin(a)) * radius, colour });
			vertices.append({ centre + sf::Vector2f(std::cos(b), std::sin(b)) * radius, colour });
		}
	}

	inline void drawPanel(sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size)
	{
		sf::RectangleShape panel(size);
		panel.setPosition(pos);
		panel.setFillColor(PANEL_FILL);
		panel.setOutlineColor(PANEL_BORDER);
		panel.setOutlineThickness(1.0f);
		window->draw(panel);
	}

	inline void drawHeader(sf::RenderWindow* window, const std::string& title, sf::Vector2f pos, float width)
	{
		drawText(window, title, { pos.x, pos.y + HEADER_TEXT_Y }, TEXT_HEADER, TEXT_PRIMARY, Align::LEFT, true);
		drawRect(window, { pos.x, pos.y + HEADER_HEIGHT - 4.0f }, { width, 1.0f }, HAIRLINE);
	}
}

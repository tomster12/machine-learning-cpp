#include "stdafx.h"
#include "GraphDock.h"
#include "UITheme.h"
#include "UITooltip.h"

namespace
{
	constexpr float DOCK_WIDTH = 240.0f;
	constexpr float DOCK_HEIGHT = 176.0f;
	constexpr float TAB_HEIGHT = 22.0f;
	constexpr float BUTTON_SIZE = 22.0f;
	constexpr float LEGEND_HEIGHT = 16.0f;
	constexpr float BAR_WIDTH = 2.0f;
	constexpr float RATE_STEP = 1.25f;
	constexpr size_t MAX_POINTS = 200;

	size_t firstVisible(size_t count) { return count > MAX_POINTS ? count - MAX_POINTS : 0; }

	void plotSeries(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const std::vector<float>& values, float maxValue, sf::Color colour)
	{
		if (values.size() < 2) return;

		sf::VertexArray line(sf::PrimitiveType::LineStrip);
		for (size_t i = 0; i < values.size(); i++)
		{
			const float x = origin.x + size.x * static_cast<float>(i) / static_cast<float>(values.size() - 1);
			const float y = origin.y + size.y * (1.0f - std::clamp(values[i] / maxValue, 0.0f, 1.0f));
			line.append(sf::Vertex{ { x, y }, colour });
		}
		window->draw(line);
	}

	void drawBaseline(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size)
	{
		theme::drawRect(window, { origin.x, origin.y + size.y }, { size.x, 1.0f }, theme::HAIRLINE);
	}

	void drawMessage(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const std::string& message)
	{
		theme::drawText(window, message, origin + size / 2.0f, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
	}

	void drawLegendEntry(sf::RenderWindow* window, float& rightX, float y, const std::string& label, sf::Color colour)
	{
		theme::drawText(window, label, { rightX, y }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::RIGHT);
		rightX -= theme::measureText(label, theme::TEXT_SMALL) + 12.0f;
		theme::drawRect(window, { rightX, y - 4.0f }, { 8.0f, 8.0f }, colour);
		rightX -= 10.0f;
	}

	bool drawButton(sf::RenderWindow* window, sf::Vector2f pos, Icon icon, bool isToggle, bool isActive, const std::string& tooltip, sf::Vector2f mouse, bool clicked)
	{
		const sf::Vector2f size(BUTTON_SIZE, BUTTON_SIZE);
		const bool hovered = sf::FloatRect(pos, size).contains(mouse);

		sf::Color fill = hovered ? theme::TILE_HOVER : theme::TILE_FILL;
		if (isActive) fill = hovered ? theme::TILE_ACTIVE_HOVER : theme::TILE_ACTIVE;
		theme::drawRect(window, pos, size, fill);
		icons::draw(window, icon, pos + size / 2.0f, 11.0f, isToggle && !isActive ? theme::TEXT_SECONDARY : theme::TEXT_PRIMARY);

		// Tooltip goes above, the dock sits on the bottom edge of the window
		if (hovered) UITooltip::set(tooltip, { pos.x, pos.y - 28.0f });
		return hovered && clicked;
	}
}

void GraphDock::render(sf::RenderWindow* window, tbml::ga::IGenepool& genepool, const std::function<float(int)>& difficulty)
{
	const float pad = theme::PADDING;
	const sf::Vector2f size(DOCK_WIDTH, DOCK_HEIGHT);
	const sf::Vector2f origin(theme::MARGIN, static_cast<float>(window->getSize().y) - theme::MARGIN - size.y);
	theme::drawPanel(window, origin, size);

	// A click only counts on the frame the button goes down
	const sf::Vector2f mouse(sf::Mouse::getPosition(*window));
	const bool isDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	const bool clicked = isDown && !isPressed;
	isPressed = isDown;

	std::vector<std::pair<Tab, std::string>> tabs = { { Tab::FITNESS, "Fitness" }, { Tab::SPREAD, "Spread" }, { Tab::MUTATION, "Mutation" } };
	if (difficulty) tabs.push_back({ Tab::DIFFICULTY, "Difficulty" });
	if (std::none_of(tabs.begin(), tabs.end(), [this](const auto& entry) { return entry.first == tab; })) tab = Tab::FITNESS;

	// Segmented tab strip along the top
	const sf::Vector2f tabOrigin(origin.x + pad, origin.y + 8.0f);
	const float tabWidth = (size.x - 2.0f * pad) / static_cast<float>(tabs.size());
	theme::drawRect(window, tabOrigin, { size.x - 2.0f * pad, TAB_HEIGHT }, theme::TILE_FILL);
	for (size_t i = 0; i < tabs.size(); i++)
	{
		const sf::FloatRect bounds({ tabOrigin.x + tabWidth * static_cast<float>(i), tabOrigin.y }, { tabWidth, TAB_HEIGHT });
		const bool hovered = bounds.contains(mouse);
		if (hovered && clicked) tab = tabs[i].first;

		const bool selected = tab == tabs[i].first;
		if (selected) theme::drawRect(window, bounds.position, bounds.size, theme::TILE_ACTIVE);
		else if (hovered) theme::drawRect(window, bounds.position, bounds.size, theme::TILE_HOVER);
		theme::drawText(window, tabs[i].second, bounds.position + bounds.size / 2.0f, theme::TEXT_SMALL, selected ? theme::TEXT_PRIMARY : theme::TEXT_SECONDARY, theme::Align::CENTER);
	}

	const sf::Vector2f contentOrigin(origin.x + pad, tabOrigin.y + TAB_HEIGHT + 8.0f);
	const sf::Vector2f contentSize(size.x - 2.0f * pad, origin.y + size.y - pad - contentOrigin.y);
	switch (tab)
	{
	case Tab::FITNESS: this->renderFitness(window, contentOrigin, contentSize, genepool.getHistory()); break;
	case Tab::SPREAD: this->renderSpread(window, contentOrigin, contentSize, genepool); break;
	case Tab::MUTATION: this->renderMutation(window, contentOrigin, contentSize, genepool, mouse, clicked); break;
	case Tab::DIFFICULTY: this->renderDifficulty(window, contentOrigin, contentSize, genepool, difficulty); break;
	}
}

void GraphDock::renderFitness(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const std::vector<tbml::ga::GenerationStats>& history) const
{
	float legendX = origin.x + size.x;
	drawLegendEntry(window, legendX, origin.y + 6.0f, "best", theme::TEXT_PRIMARY);
	drawLegendEntry(window, legendX, origin.y + 6.0f, "avg", theme::TEXT_SECONDARY);

	const sf::Vector2f plotOrigin(origin.x, origin.y + LEGEND_HEIGHT);
	const sf::Vector2f plotSize(size.x, size.y - LEGEND_HEIGHT);
	drawBaseline(window, plotOrigin, plotSize);
	if (history.size() < 2)
	{
		drawMessage(window, plotOrigin, plotSize, "Waiting for generation 2");
		return;
	}

	// Plot the most recent generations, scaled to the best seen in that window
	std::vector<float> best;
	std::vector<float> average;
	float maxValue = 1.0f;
	for (size_t i = firstVisible(history.size()); i < history.size(); i++)
	{
		best.push_back(history[i].best);
		average.push_back(history[i].average);
		maxValue = std::max(maxValue, history[i].best);
	}
	plotSeries(window, plotOrigin, plotSize, average, maxValue, theme::TEXT_SECONDARY);
	plotSeries(window, plotOrigin, plotSize, best, maxValue, theme::TEXT_PRIMARY);
	theme::drawText(window, global::formatFloat(maxValue, 0), { plotOrigin.x + 4.0f, plotOrigin.y + 10.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY);
}

void GraphDock::renderSpread(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const tbml::ga::IGenepool& genepool)
{
	// Every agent this generation, fittest first, with the carried over elites picked out
	const size_t count = genepool.getPopulationSize();
	const size_t eliteCount = genepool.getGenerationNumber() > 1 ? genepool.getEliteCount() : 0;

	float legendX = origin.x + size.x;
	if (eliteCount > 0) drawLegendEntry(window, legendX, origin.y + 6.0f, "elite (" + std::to_string(eliteCount) + ")", theme::ELITE);
	drawLegendEntry(window, legendX, origin.y + 6.0f, "rest", theme::TEXT_SECONDARY);

	const sf::Vector2f plotOrigin(origin.x, origin.y + LEGEND_HEIGHT);
	const sf::Vector2f plotSize(size.x, size.y - LEGEND_HEIGHT);
	drawBaseline(window, plotOrigin, plotSize);
	if (count == 0) return;

	ranked.clear();
	for (size_t i = 0; i < count; i++) ranked.emplace_back(genepool.getAgentFitness(i), i < eliteCount);
	std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
	const float maxValue = std::max(ranked.front().first, 1e-6f);

	// Each bar covers a slice of the ranking: its average height, coloured if it holds an elite
	const size_t bars = std::max<size_t>(1, static_cast<size_t>(plotSize.x / BAR_WIDTH));
	sf::VertexArray vertices(sf::PrimitiveType::Triangles);
	const sf::Color restColour(theme::TEXT_SECONDARY.r, theme::TEXT_SECONDARY.g, theme::TEXT_SECONDARY.b, 190);
	for (size_t bar = 0; bar < bars; bar++)
	{
		const size_t begin = bar * count / bars;
		const size_t end = std::max(begin + 1, (bar + 1) * count / bars);
		float total = 0.0f;
		bool hasElite = false;
		for (size_t i = begin; i < end && i < count; i++)
		{
			total += ranked[i].first;
			hasElite |= ranked[i].second;
		}

		const float height = std::max(1.0f, plotSize.y * (total / static_cast<float>(end - begin)) / maxValue);
		const float x = plotOrigin.x + BAR_WIDTH * static_cast<float>(bar);
		const float top = plotOrigin.y + plotSize.y - height;
		const sf::Color colour = hasElite ? theme::ELITE : restColour;
		const sf::Vertex a{ { x, top }, colour };
		const sf::Vertex b{ { x + BAR_WIDTH, top }, colour };
		const sf::Vertex c{ { x + BAR_WIDTH, plotOrigin.y + plotSize.y }, colour };
		const sf::Vertex d{ { x, plotOrigin.y + plotSize.y }, colour };
		vertices.append(a);
		vertices.append(b);
		vertices.append(c);
		vertices.append(a);
		vertices.append(c);
		vertices.append(d);
	}
	window->draw(vertices);

	theme::drawText(window, global::formatFloat(maxValue, 0), { plotOrigin.x + plotSize.x - 4.0f, plotOrigin.y + 10.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::RIGHT);
	theme::drawText(window, "best", { plotOrigin.x + 4.0f, plotOrigin.y + 10.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY);
}

void GraphDock::renderMutation(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, tbml::ga::IGenepool& genepool, sf::Vector2f mouse, bool clicked) const
{
	// Controls: lower, raise, back to the starting rate, and let the genepool adapt it
	const float rate = genepool.getMutationRate();
	const float step = BUTTON_SIZE + 2.0f;
	if (drawButton(window, { origin.x, origin.y }, Icon::MINUS, false, false, "Lower mutation rate", mouse, clicked)) genepool.setMutationRate(rate / RATE_STEP);
	if (drawButton(window, { origin.x + step, origin.y }, Icon::PLUS, false, false, "Raise mutation rate", mouse, clicked)) genepool.setMutationRate(rate * RATE_STEP);
	if (drawButton(window, { origin.x + 2.0f * step, origin.y }, Icon::RESET, false, false, "Reset to the starting rate", mouse, clicked)) genepool.resetMutationRate();

	const bool adaptive = genepool.getAdaptiveMutation();
	if (drawButton(window, { origin.x + 3.0f * step, origin.y }, Icon::AUTO, true, adaptive, "Auto: raise when stuck, lower while improving", mouse, clicked)) genepool.setAdaptiveMutation(!adaptive);
	theme::drawText(window, global::formatFloat(rate, 3), { origin.x + size.x, origin.y + BUTTON_SIZE / 2.0f }, theme::TEXT_BODY, theme::TEXT_PRIMARY, theme::Align::RIGHT);

	const sf::Vector2f plotOrigin(origin.x, origin.y + BUTTON_SIZE + 8.0f);
	const sf::Vector2f plotSize(size.x, size.y - BUTTON_SIZE - 8.0f);
	drawBaseline(window, plotOrigin, plotSize);

	// Rate each generation was bred with, ending on the live rate so manual changes show immediately
	const std::vector<tbml::ga::GenerationStats>& history = genepool.getHistory();
	std::vector<float> rates;
	for (size_t i = firstVisible(history.size()); i < history.size(); i++) rates.push_back(history[i].mutationRate);
	rates.push_back(rate);
	if (rates.size() < 2)
	{
		drawMessage(window, plotOrigin, plotSize, "Waiting for generation 2");
		return;
	}

	const float maxValue = *std::max_element(rates.begin(), rates.end()) * 1.15f;
	plotSeries(window, plotOrigin, plotSize, rates, maxValue, theme::INFO);
	theme::drawText(window, global::formatFloat(maxValue, 3), { plotOrigin.x + 4.0f, plotOrigin.y + 10.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY);
}

void GraphDock::renderDifficulty(sf::RenderWindow* window, sf::Vector2f origin, sf::Vector2f size, const tbml::ga::IGenepool& genepool, const std::function<float(int)>& difficulty) const
{
	// One point per generation including the one in progress
	const size_t generations = genepool.getHistory().size() + 1;
	std::vector<float> values;
	for (size_t i = firstVisible(generations); i < generations; i++) values.push_back(difficulty(static_cast<int>(i) + 1));

	float legendX = origin.x + size.x;
	drawLegendEntry(window, legendX, origin.y + 6.0f, "difficulty", theme::WARNING);
	theme::drawText(window, global::formatFloat(values.back() * 100.0f, 0) + "%", { origin.x, origin.y + 6.0f }, theme::TEXT_BODY, theme::TEXT_PRIMARY);

	const sf::Vector2f plotOrigin(origin.x, origin.y + LEGEND_HEIGHT);
	const sf::Vector2f plotSize(size.x, size.y - LEGEND_HEIGHT);
	drawBaseline(window, plotOrigin, plotSize);
	if (values.size() < 2)
	{
		drawMessage(window, plotOrigin, plotSize, "Waiting for generation 2");
		return;
	}

	plotSeries(window, plotOrigin, plotSize, values, 1.0f, theme::WARNING);
	theme::drawText(window, "100%", { plotOrigin.x + 4.0f, plotOrigin.y + 10.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY);
}

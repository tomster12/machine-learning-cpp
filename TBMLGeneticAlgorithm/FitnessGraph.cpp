#include "stdafx.h"
#include "FitnessGraph.h"
#include "UITheme.h"

void FitnessGraph::render(sf::RenderWindow* window, const std::vector<tbml::ga::GenerationStats>& history) const
{
	const float pad = theme::PADDING;
	const sf::Vector2f size(theme::PANEL_WIDTH, 150.0f);
	const sf::Vector2f origin(theme::MARGIN, static_cast<float>(window->getSize().y) - theme::MARGIN - size.y);
	theme::drawPanel(window, origin, size);

	const sf::Color bestColour = theme::TEXT_PRIMARY;
	const sf::Color averageColour = theme::TEXT_SECONDARY;
	const float headerY = origin.y + theme::HEADER_INSET + theme::HEADER_TEXT_Y;
	theme::drawHeader(window, "Fitness", { origin.x + pad, origin.y + theme::HEADER_INSET }, size.x - 2.0f * pad);

	// Legend, right aligned: [swatch] avg  [swatch] best
	float legendX = origin.x + size.x - pad;
	for (const auto& entry : { std::make_pair("best", bestColour), std::make_pair("avg", averageColour) })
	{
		theme::drawText(window, entry.first, { legendX, headerY }, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::RIGHT);
		legendX -= theme::measureText(entry.first, theme::TEXT_SMALL) + 12.0f;
		theme::drawRect(window, { legendX, headerY - 4.0f }, { 8.0f, 8.0f }, entry.second);
		legendX -= 10.0f;
	}

	const sf::Vector2f plotOrigin(origin.x + pad, origin.y + theme::HEADER_INSET + theme::HEADER_HEIGHT + theme::GAP);
	const sf::Vector2f plotSize(size.x - 2.0f * pad, size.y - (plotOrigin.y - origin.y) - pad);

	if (history.size() < 2)
	{
		theme::drawText(window, "Waiting for generation 2", plotOrigin + plotSize / 2.0f, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
		return;
	}

	// Plot the most recent generations, scaled to the best seen in that window
	const size_t count = std::min<size_t>(history.size(), 200);
	const size_t first = history.size() - count;
	float maxValue = 1.0f;
	for (size_t i = first; i < history.size(); i++) maxValue = std::max(maxValue, history[i].best);

	sf::VertexArray bestLine(sf::PrimitiveType::LineStrip);
	sf::VertexArray averageLine(sf::PrimitiveType::LineStrip);
	for (size_t i = 0; i < count; i++)
	{
		const float x = plotOrigin.x + plotSize.x * static_cast<float>(i) / static_cast<float>(count - 1);
		const tbml::ga::GenerationStats& stats = history[first + i];
		bestLine.append(sf::Vertex{ { x, plotOrigin.y + plotSize.y * (1.0f - stats.best / maxValue) }, bestColour });
		averageLine.append(sf::Vertex{ { x, plotOrigin.y + plotSize.y * (1.0f - stats.average / maxValue) }, averageColour });
	}
	window->draw(averageLine);
	window->draw(bestLine);
	theme::drawText(window, global::formatFloat(maxValue, 0), { plotOrigin.x + 4.0f, plotOrigin.y + 10.0f }, theme::TEXT_SMALL, theme::TEXT_SECONDARY);
}

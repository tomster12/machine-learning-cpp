#include "stdafx.h"
#include <random>
#include "CommonImpl.h"
#include "NNPoleBalancerScenario.h"
#include "UIStatsPanel.h"
#include "UITheme.h"

namespace
{
    constexpr float TRACK_LIMIT = 1.0f;
    constexpr float ANGLE_LIMIT = 0.21f;
    constexpr float TIME_LIMIT = 30.0f;
    constexpr float RAD_TO_DEG = 57.2957795f;
    constexpr int RAMP_GENERATIONS = 40;

    struct StartState
    {
        float angle = 0.0f;
        float position = 0.0f;
    };

    float getGenerationDifficulty(int generation)
    {
        return std::min(1.0f, static_cast<float>(generation - 1) / static_cast<float>(RAMP_GENERATIONS));
    }

    StartState getGenerationStart(int generation)
    {
        std::mt19937 rng(static_cast<unsigned int>(generation) * 7919u + 17u);
        auto random = [&](float low, float high)
        { return low + (high - low) * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng); };

        // Early generations start almost upright on the centre, later ones lean harder in either direction from off to the side
        const float difficulty = getGenerationDifficulty(generation);
        StartState start;
        start.angle = random(-1.0f, 1.0f) * std::lerp(0.03f, 0.1f, difficulty);
        start.position = random(-1.0f, 1.0f) * std::lerp(0.0f, 0.5f, difficulty);
        return start;
    }
}

NNPoleBalancerScenario::NNPoleBalancerScenario()
    : nnRenderer({{"position", TRACK_LIMIT}, {"velocity", 2.0f}, {"angle", ANGLE_LIMIT}, {"ang. velocity", 1.0f}}, {"force"})
{
    this->genepool = std::make_shared<PoleGenepool>([]() {
		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
			std::make_shared<tbml::nn::Layer::Dense>(4, 1),
            std::make_shared<tbml::nn::Layer::TanH>()}));
		}, nullptr);

    // Every agent in a generation starts from the same tilt and offset, which changes each generation as it gets harder
    this->genepool->setCreateAgentFn([this](PoleGenepool::GenomeCPtr genome)
                                     {
		const StartState start = getGenerationStart(this->genepool->getGenerationNumber() + 1);
		return std::make_unique<NNPoleBalancerAgent>(
			std::move(genome),
			1.0f, 0.1f, 1.0f, 1.0f, TRACK_LIMIT, ANGLE_LIMIT, TIME_LIMIT,
			start.angle, start.position
		); });

    // Only one neuron to recombine, so adapting the mutation rate is what keeps it from stalling
    this->genepool->setAdaptiveMutation(true);
    this->genepool->configThreading(false, true, false);
    this->genepool->resetGenepool(1000, 0.1f);
}

float NNPoleBalancerScenario::getGenerationDifficulty(int generation) const
{
    return getGenerationDifficulty(generation);
}

void NNPoleBalancerScenario::initAgentStats(UIStatsPanel &panel)
{
    panel.row("Time", [this]()
              { return global::formatFloat(static_cast<float>(stats.step) * NNPoleBalancerAgent::TIME_STEP, 1) + " / " + global::formatFloat(TIME_LIMIT, 0) + " s"; });
    panel.row("Cart position", [this]()
              { return global::formatFloat(getEliteAgent()->getCartPosition(), 2) + " m"; });
    panel.row("Pole angle", [this]()
              { return global::formatFloat(getEliteAgent()->getPoleAngle() * RAD_TO_DEG, 1) + " deg"; });
    panel.row("Force", [this]()
              { return global::formatFloat(getEliteAgent()->getPush() * 100.0f, 0) + "%"; });

    panel.header("Start state");
    panel.row("Pole tilt", [this]()
              { return global::formatFloat(getEliteAgent()->getStartAngle() * RAD_TO_DEG, 1) + " deg"; });
    panel.row("Cart offset", [this]()
              { return global::formatFloat(getEliteAgent()->getStartPosition(), 2) + " m"; });
    panel.row("Difficulty", [this]()
              { return global::formatFloat(this->getGenerationDifficulty(genepool->getGenerationNumber()) * 100.0f, 0) + "%"; });

    // Swatches double as the legend for cart colours
    panel.header("Outcomes");
    panel.row("Balancing", [this]()
              { return std::to_string(stats.balancing) + " / " + std::to_string(stats.total); }, &theme::CART_BALANCING);
    panel.row("Fell over", [this]()
              { return std::to_string(stats.fell); }, &theme::CART_FELL);
    panel.row("Off track", [this]()
              { return std::to_string(stats.offTrack); }, &theme::CART_OFF_TRACK);
    panel.row("Survived", [this]()
              { return std::to_string(stats.survived); }, &theme::CART_SURVIVED);
}

void NNPoleBalancerScenario::updateDisplay()
{
    Scenario::updateDisplay();

    if (!genepool->getGenepoolInitialized()) return;

    this->updateStats();
    this->updateTrail();
}

void NNPoleBalancerScenario::updateStats()
{
    stats = PopulationStats();
    for (const auto &agent : *genepool->getAgentPopulation())
    {
        stats.total++;
        stats.step = std::max(stats.step, agent->getStep());

        switch (agent->getOutcome())
        {
        case PoleOutcome::BALANCING:
            stats.balancing++;
            break;
        case PoleOutcome::FELL:
            stats.fell++;
            break;
        case PoleOutcome::OFF_TRACK:
            stats.offTrack++;
            break;
        case PoleOutcome::SURVIVED:
            stats.survived++;
            break;
        }
    }
}

void NNPoleBalancerScenario::updateTrail()
{
    trail.follow(getEliteIndex(), genepool->getGenerationNumber(), getEliteAgent()->getTipPosition());
}

void NNPoleBalancerScenario::renderWorld(sf::RenderWindow *window)
{
    this->renderBackdrop(window);
    this->renderTrack(window);
}

void NNPoleBalancerScenario::renderBackdrop(sf::RenderWindow *window) const
{
    // Vertical gradient across whatever the camera can see, with the floor under the rail
    const sf::View &view = window->getView();
    const sf::Vector2f topLeft = view.getCenter() - view.getSize() / 2.0f;
    const sf::Vector2f size = view.getSize();
    const sf::Color top(12, 14, 22);
    const sf::Color bottom(30, 34, 48);

    sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
    sky.append({topLeft, top});
    sky.append({{topLeft.x + size.x, topLeft.y}, top});
    sky.append({{topLeft.x, topLeft.y + size.y}, bottom});
    sky.append({topLeft + size, bottom});
    window->draw(sky);

    const float floorTop = NNPoleBalancerAgent::RAIL_Y + 8.0f;
    theme::drawRect(window, {topLeft.x, floorTop}, {size.x, std::max(0.0f, topLeft.y + size.y - floorTop)}, sf::Color(16, 18, 26));
    theme::drawRect(window, {topLeft.x, floorTop}, {size.x, 1.0f}, sf::Color(255, 255, 255, 28));
}

void NNPoleBalancerScenario::renderTrack(sf::RenderWindow *window) const
{
    const float centreX = NNPoleBalancerAgent::ORIGIN_X;
    const float railY = NNPoleBalancerAgent::RAIL_Y;
    const float unit = NNPoleBalancerAgent::METRE_TO_UNIT;
    const float cartHalf = NNPoleBalancerAgent::CART_WIDTH / 2.0f;
    const float railHalf = TRACK_LIMIT * unit + cartHalf + 40.0f;
    const float time = clock.getElapsedTime().asSeconds();

    // Rail with a highlight along the top and a tick every quarter metre
    theme::drawRect(window, {centreX - railHalf, railY}, {railHalf * 2.0f, 8.0f}, sf::Color(70, 76, 92));
    theme::drawRect(window, {centreX - railHalf, railY}, {railHalf * 2.0f, 2.0f}, sf::Color(150, 158, 178));
    for (int i = -4; i <= 4; i++)
    {
        const bool major = i % 4 == 0;
        theme::drawRect(window, {centreX + static_cast<float>(i) * 0.25f * unit - 1.0f, railY + 8.0f}, {2.0f, major ? 12.0f : 6.0f}, sf::Color(255, 255, 255, major ? 90 : 40));
    }
    theme::drawText(window, "-1 m", {centreX - unit, railY + 30.0f}, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
    theme::drawText(window, "0", {centreX, railY + 30.0f}, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);
    theme::drawText(window, "+1 m", {centreX + unit, railY + 30.0f}, theme::TEXT_SMALL, theme::TEXT_SECONDARY, theme::Align::CENTER);

    // Bumpers where the cart runs out of track, with beacons blinking out of phase
    sf::CircleShape beacon(5.0f);
    beacon.setOrigin({5.0f, 5.0f});
    for (int side = -1; side <= 1; side += 2)
    {
        const float x = centreX + static_cast<float>(side) * (TRACK_LIMIT * unit + cartHalf + 6.0f);
        theme::drawRect(window, {x - 5.0f, railY - 46.0f}, {10.0f, 46.0f}, sf::Color(theme::NEGATIVE.r, theme::NEGATIVE.g, theme::NEGATIVE.b, 200));

        const float blink = 0.5f + 0.5f * std::sin(time * 4.0f + static_cast<float>(side) * 1.6f);
        beacon.setPosition({x, railY - 54.0f});
        beacon.setFillColor(sf::Color(255, 120, 110, theme::toAlpha(90.0f + 165.0f * blink)));
        window->draw(beacon);
    }
}

void NNPoleBalancerScenario::renderWedge(sf::RenderWindow *window, const NNPoleBalancerAgent &elite) const
{
    // Fan of angles the pole can lean through before it counts as fallen, fading out away from the pivot
    const sf::Vector2f pivot = elite.getPivotPosition();
    const float radius = 460.0f;
    const int segments = 12;

    sf::VertexArray fan(sf::PrimitiveType::TriangleFan);
    fan.append({pivot, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 36)});
    for (int i = 0; i <= segments; i++)
    {
        const float angle = std::lerp(-ANGLE_LIMIT, ANGLE_LIMIT, static_cast<float>(i) / static_cast<float>(segments));
        fan.append({pivot + sf::Vector2f(std::sin(angle), -std::cos(angle)) * radius, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 0)});
    }
    window->draw(fan);

    sf::VertexArray edges(sf::PrimitiveType::Triangles);
    for (int side = -1; side <= 1; side += 2)
    {
        const float angle = static_cast<float>(side) * ANGLE_LIMIT;
        theme::appendLine(edges, pivot, pivot + sf::Vector2f(std::sin(angle), -std::cos(angle)) * radius * 0.8f, 1.5f, sf::Color(theme::INFO.r, theme::INFO.g, theme::INFO.b, 60));
    }
    window->draw(edges);
}

void NNPoleBalancerScenario::renderAgents(sf::RenderWindow *window)
{
    if (getView() == AgentView::NONE) return;

    const NNPoleBalancerAgent *elite = getEliteAgent().get();

    if (!elite->getFinished()) this->renderWedge(window, *elite);
    trail.render(window);

    // Whole pack in one draw call, elite is drawn separately on top
    if (getView() == AgentView::ALL)
    {
        packVertices.clear();
        for (const auto &agent : *genepool->getAgentPopulation())
        {
            if (agent.get() != elite) agent->appendPackVertices(packVertices);
        }
        window->draw(packVertices);
    }

    elite->renderElite(window);
}

void NNPoleBalancerScenario::renderOverlay(sf::RenderWindow *window)
{
    const PoleGenepool::AgentPtr elite = getEliteAgent();
    nnRenderer.render(window, elite->getBrain(), elite->getNetInput());
}

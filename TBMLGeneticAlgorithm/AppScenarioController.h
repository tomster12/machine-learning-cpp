#pragma once

#include "stdafx.h"
#include "GenepoolSimulation.h"
#include "UIManager.h"
#include "Scenario.h"

class AppScenarioController
{
public:
	AppScenarioController() = default;
	AppScenarioController(ScenarioUPtr&& scenario, sf::RenderWindow* window);

	void initUI(sf::RenderWindow* window);
	void handleEvent(const sf::Event& event);
	void update();
	void render(sf::RenderWindow* window);
	void fullEvaluate();
	void iterate();
	void setEvaluating(bool v) { evaluating = v; }
	void setFullEvaluating(bool v) { toFullEvaluate = v; }
	void setIterating(bool v) { iterating = v; }
	void setFastMode(bool v) { isFastMode = v; }

private:
	void runFast();
	void changeSpeed(int direction);
	std::string getSpeedText() const;

	static constexpr float SPEEDS[] = { 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f };
	static constexpr size_t DEFAULT_SPEED_INDEX = 4;
	static constexpr int FAST_FRAME_MS = 50;

	std::unique_ptr<UIManager> ui = nullptr;
	ScenarioUPtr scenario = nullptr;
	tbml::ga::IGenepoolPtr genepool = nullptr;
	sf::Clock frameClock;
	float fps = 0.0f;
	size_t speedIndex = DEFAULT_SPEED_INDEX;
	float stepAccumulator = 0.0f;
	bool isFastMode = false;
	bool evaluating = false;
	bool toFullEvaluate = false;
	bool iterating = false;
};

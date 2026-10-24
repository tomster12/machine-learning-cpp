#include "stdafx.h"
#include "App.h"
#include "TbmlGlobal.h"
#include "NNDriverScenario.h"
#include "NNPoleBalancerScenario.h"
#include "VectorListTargetScenario.h"
#include "NNTargetScenario.h"
#include "LanderScenario.h"

#define SCENARIO 5

App::~App()
{
	delete window;
}

void App::run()
{
	this->initialize();

	while (window->isOpen())
	{
		update();
		render();
	}
}

void App::initialize()
{
	global::initialize();
	tbml::setOmpThreads(1);

	sf::VideoMode mode({ 1400, 1000 });
	window = new sf::RenderWindow(mode, "Genetic Algorithm", sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close);
	window->setFramerateLimit(60);
	window->setVerticalSyncEnabled(false);

	#if SCENARIO == 1
	ScenarioUPtr scenario = std::make_unique<NNTargetScenario>();
	#elif SCENARIO == 2
	ScenarioUPtr scenario = std::make_unique<NNDriverScenario>();
	#elif SCENARIO == 3
	ScenarioUPtr scenario = std::make_unique<NNPoleBalancerScenario>();
	#elif SCENARIO == 4
	ScenarioUPtr scenario = std::make_unique<VectorListTargetScenario>();
	#elif SCENARIO == 5
	ScenarioUPtr scenario = std::make_unique<LanderScenario>();
	#endif

	controller = std::make_unique<AppScenarioController>(std::move(scenario), window);
}

void App::update()
{
	while (const auto event = window->pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			window->close();
		}

		controller->handleEvent(*event);
	}

	controller->update();
}

void App::render()
{
	window->clear();

	controller->render(window);

	window->display();
}

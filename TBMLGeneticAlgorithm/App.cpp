#include "stdafx.h"
#include "App.h"
#include "TbmlGlobal.h"
#include "NNDriverScenario.h"
#include "NNPoleBalancerScenario.h"
#include "VectorListTargetScenario.h"
#include "NNTargetScenario.h"

#define SCENARIO 0

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
	window = new sf::RenderWindow(mode, "Genetic Algorithm", sf::Style::Titlebar | sf::Style::Close);
	window->setFramerateLimit(60);

	#if SCENARIO == 0
	IAppScenarioUPtr scenario = std::make_unique<NNTargetScenario>();
	#elif SCENARIO == 1
	IAppScenarioUPtr scenario = std::make_unique<NNDriverScenario>();
	#elif SCENARIO == 2
	IAppScenarioUPtr scenario = std::make_unique<NNPoleBalancerScenario>();
	#elif SCENARIO == 3
	IAppScenarioUPtr scenario = std::make_unique<VectorListTargetScenario>();
	#endif

	controller = std::make_unique<AppScenarioController>(std::move(scenario), window);
}

void App::update()
{
	dt = dtClock.restart().asSeconds();

	while (const auto event = window->pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			window->close();
		}
	}

	controller->update();
}

void App::render()
{
	window->clear();

	controller->render(window);

	window->display();
}

#include "stdafx.h"
#include "App.h"
#include "TbmlGlobal.h"
#include "NNDriverScenario.h"
#include "NNPoleBalancerScenario.h"
#include "VectorListTargetScenario.h"
#include "NNTargetScenario.h"
#include "LanderScenario.h"
#include "WalkerScenario.h"
#include "DogfightScenario.h"

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

	std::vector<ScenarioEntry> entries;
	entries.push_back({ "Vector List", [] { return std::make_unique<VectorListTargetScenario>(); } });
	entries.push_back({ "NN Target", [] { return std::make_unique<NNTargetScenario>(); } });
	entries.push_back({ "Pole Balancer", [] { return std::make_unique<NNPoleBalancerScenario>(); } });
	entries.push_back({ "Driver", [] { return std::make_unique<NNDriverScenario>(); } });
	entries.push_back({ "Lander", [] { return std::make_unique<LanderScenario>(); } });
	entries.push_back({ "Walker", [] { return std::make_unique<WalkerScenario>(); } });
	entries.push_back({ "Dogfight", [] { return std::make_unique<DogfightScenario>(); } });

	const size_t startIndex = entries.size() - 1;
	controller = std::make_unique<AppScenarioController>(std::move(entries), startIndex, window);
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

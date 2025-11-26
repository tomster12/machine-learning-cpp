#include "stdafx.h"
#include "App.h"
#include "UIToggleButton.h"
#include "UIButton.h"
#include "UIDynamicText.h"
#include "TbmlGlobal.h"

App::~App()
{
	delete window;
}

int App::run(IAppGenepoolPtr&& genepool)
{
	if (initialize(std::move(genepool)) != 0) return 1;

	while (window->isOpen())
	{
		update();
		render();
	}

	return 0;
}

int App::initialize(IAppGenepoolPtr&& genepool)
{
	if (global::initialize() != 0) return 1;

	tbml::setOmpThreads(1);

	sf::VideoMode mode({ 1400, 1000 });
	window = new sf::RenderWindow(mode, "Genetic Algorithm", sf::Style::Titlebar | sf::Style::Close);
	window->setFramerateLimit(60);

	genepool->logInformation();
	controller = std::make_unique<AppGenepoolController>(std::move(genepool));
	ui = std::make_unique<UIManager>();

	setupUI();

	return 0;
}

void App::setupUI()
{
	this->ui = std::make_unique<UIManager>();
	float osp = 6.0f;
	float sp = 6.0f;
	float sz = 30.0f;

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(this->window, { osp + sp + 0 * (sp + sz), osp + sp + 0 * (sp + sz) }, { sz, sz }, "assets/autoEvaluate.png", false,
		[&](bool toggled) { this->controller->setEvaluate(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(this->window, { osp + sp + 0 * (sp + sz), osp + sp + 1 * (sp + sz) }, { sz, sz }, "assets/autoFullEvaluate.png", false,
		[&](bool toggled) { this->controller->setFullEvaluate(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIButton(this->window, { osp + sp + 1 * (sp + sz), osp + sp + 0 * (sp + sz) }, { sz, sz }, "assets/iterate.png",
		[&]() { this->controller->iterateGeneration(); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(this->window, { osp + sp + 1 * (sp + sz), osp + sp + 1 * (sp + sz) }, { sz, sz }, "assets/autoIterate.png", false,
		[&](bool toggled) { this->controller->setAutoIterate(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(this->window, { osp + sp + 2 * (sp + sz), osp + sp + 0 * (sp + sz) }, { sz, sz }, "assets/show.png", true,
		[&](bool toggled) { this->controller->setShowVisuals(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIDynamicText(this->window, { osp + sp * 1.2f, osp + sp + osp + 2 * (sp + sz) + 0 }, 15,
		[&]() { return std::string("Generation: ") + std::to_string(this->controller->getGenepool()->getGenerationNumber()); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIDynamicText(this->window, { osp + sp * 1.2f, osp + sp + osp + 2 * (sp + sz) + 20 }, 15,
		[&]() { return std::string("Evaluated: ") + std::string(this->controller->getGenepool()->getGenerationEvaluated() ? "True" : "False"); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIDynamicText(this->window, { osp + sp * 1.2f, osp + sp + osp + 2 * (sp + sz) + 40 }, 15,
		[&]() { return std::string("Best Fitness: ") + std::to_string(this->controller->getGenepool()->getBestFitness()); })));
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
	ui->update();
}

void App::render()
{
	window->clear();

	controller->render(window);
	ui->render(window);

	window->display();
}

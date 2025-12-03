#include "stdafx.h"
#include "AppScenarioController.h"
#include "UIToggleButton.h"
#include "UIButton.h"
#include "UIDynamicText.h"

AppScenarioController::AppScenarioController(IAppScenarioUPtr&& scenario, sf::RenderWindow* window)
	: scenario(std::move(scenario))
{
	this->genepool = this->scenario->getGenepool();
	this->initUI(window);
}

void AppScenarioController::initUI(sf::RenderWindow* window)
{
	this->ui = std::make_unique<UIManager>();

	float osp = 6.0f;
	float sp = 6.0f;
	float sz = 30.0f;

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(window, { osp + sp + 0 * (sp + sz), osp + sp + 0 * (sp + sz) }, { sz, sz }, "assets/buttons_evaluating.png", false,
		[&](bool toggled) { this->setEvaluating(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIButton(window, { osp + sp + 1 * (sp + sz), osp + sp + 0 * (sp + sz) }, { sz, sz }, "assets/buttons_fullEvaluate.png",
		[&]() { this->fullEvaluate(); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(window, { osp + sp + 2 * (sp + sz), osp + sp + 0 * (sp + sz) }, { sz, sz }, "assets/buttons_fullEvaluating.png", false,
		[&](bool toggled) { this->setFullEvaluating(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIButton(window, { osp + sp + 0 * (sp + sz), osp + sp + 1 * (sp + sz) }, { sz, sz }, "assets/buttons_iterate.png",
		[&]() { this->iterate(); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(window, { osp + sp + 1 * (sp + sz), osp + sp + 1 * (sp + sz) }, { sz, sz }, "assets/buttons_iterating.png", false,
		[&](bool toggled) { this->setIterating(toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIToggleButton(window, { osp + sp + 2 * (sp + sz), osp + sp + 1 * (sp + sz) }, { sz, sz }, "assets/buttons_hide.png", false,
		[&](bool toggled) { this->setShowVisuals(!toggled); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIDynamicText(window, { osp + sp * 1.2f, osp + sp + osp + 2 * (sp + sz) + 0 }, 15,
		[&]() { return std::string("Generation: ") + std::to_string(this->genepool->getGenerationNumber()); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIDynamicText(window, { osp + sp * 1.2f, osp + sp + osp + 2 * (sp + sz) + 20 }, 15,
		[&]() { return std::string("Evaluated: ") + std::string(this->genepool->getGenerationEvaluated() ? "True" : "False"); })));

	this->ui->addElement(std::shared_ptr<UIElement>(new UIDynamicText(window, { osp + sp * 1.2f, osp + sp + osp + 2 * (sp + sz) + 40 }, 15,
		[&]() { return std::string("Best Fitness: ") + std::to_string(this->genepool->getBestFitness()); })));

	this->scenario->initUI(window, ui.get());
}

void AppScenarioController::update()
{
	if (!genepool->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");

	this->ui->update();

	scenario->update();

	if (!genepool->getGenerationEvaluated() && evaluating) genepool->evaluateGeneration(!toFullEvaluate);

	if (genepool->getGenerationEvaluated() && iterating) genepool->iterateGeneration();
}

void AppScenarioController::render(sf::RenderWindow* window)
{
	if (this->toShowVisuals)
	{
		if (!genepool->getGenepoolInitialized()) throw std::runtime_error("Genepool not initialized.");

		scenario->render(window);
	}

	this->ui->render(window);
}

void AppScenarioController::fullEvaluate()
{
	if (!genepool->getGenepoolInitialized()) return;

	if (genepool->getGenerationEvaluated()) return;

	genepool->evaluateGeneration(false);
}

void AppScenarioController::iterate()
{
	if (!genepool->getGenepoolInitialized()) return;

	if (!genepool->getGenerationEvaluated()) return;

	genepool->iterateGeneration();
}

#pragma once

#include "stdafx.h"
#include "GenepoolSimulation.h"

class GenepoolController
{
public:
	GenepoolController() = default;
	GenepoolController(tbml::ga::IGenepoolPtr&& pool);

	void update();
	void render(sf::RenderWindow* window);
	void iterateGeneration();

	void setEvaluate(bool v);
	void setFullEvaluate(bool v);
	void setAutoIterate(bool v);
	void setShowVisuals(bool v);

	tbml::ga::IGenepoolPtr getGenepool() const;

private:
	tbml::ga::IGenepoolPtr genepool = nullptr;
	bool toEvaluate = false;
	bool toFullEvaluate = false;
	bool toAutoIterate = false;
};

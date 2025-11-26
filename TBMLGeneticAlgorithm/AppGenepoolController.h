#pragma once

#include "stdafx.h"
#include "GenepoolSimulation.h"

class IAppGenepool : public virtual tbml::ga::IGenepool
{
public:
	virtual void render(sf::RenderWindow* window) = 0;
};

using IAppGenepoolPtr = std::shared_ptr<IAppGenepool>;

class AppGenepoolController
{
public:
	AppGenepoolController() = default;
	AppGenepoolController(IAppGenepoolPtr&& pool);

	void update();
	void render(sf::RenderWindow* window);
	void iterateGeneration();
	void setEvaluate(bool v);
	void setFullEvaluate(bool v);
	void setAutoIterate(bool v);
	void setShowVisuals(bool v);
	IAppGenepoolPtr getGenepool() const;

private:
	IAppGenepoolPtr controller = nullptr;
	bool toEvaluate = false;
	bool toFullEvaluate = false;
	bool toAutoIterate = false;
};

#include "stdafx.h"
#include "VectorListTargetGenepool.h"
#include "VectorListTargetApp.h"
#include "VectorListTarget.h"

tbml::ga::IGenepoolPtr VectorListTargetApp::createGenepool()
{
	auto* pool = new VectorListTargetGenepool(
		[]() { return std::make_shared<VectorListGenome>(500); },
		nullptr,
		sf::Vector2f{ 700.0f, 100.0f },
		20.0f
	);

	pool->setCreateAgentFn([=](VectorListTargetGenepool::GenomeCPtr data)
	{
		return std::make_unique<VectorListTargetAgent>(
			std::move(data),
			pool,
			sf::Vector2f{ 700.0f, 600.0f },
			4.0f,
			4.0f
		);
	});

	pool->configThreading(false, true, false);
	pool->resetGenepool(1000, 0.04f);

	return tbml::ga::IGenepoolPtr(pool);
}

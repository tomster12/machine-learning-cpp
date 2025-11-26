#include "stdafx.h"

//#include "NNTargetApp.h"
//#include "NNTargetGenepool.h"
//#include "NNTargetAgent.h"
//
//tbml::ga::IGenepoolPtr NNTargetApp::createGenepool()
//{
//	auto* pool = new NNTargetGenepool([]()
//	{
//		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
//			std::make_shared<tbml::nn::Layer::Dense>(4, 2),
//			std::make_shared<tbml::nn::Layer::TanH>() }));
//	},
//		nullptr,
//		{ {300,150}, {1100,400}, {450,850}, {700,320} },
//		4.0f
//	);
//
//	pool->setCreateAgentFn([=](NNTargetGenepool::GenomeCPtr data)
//	{
//		return std::make_unique<NNTargetAgent>(
//			std::move(data), pool, sf::Vector2f{ 700,850 },
//			2.0f, 400.0f, 0.99f, 3000);
//	});
//
//	pool->configThreading(false, true, false);
//	pool->resetGenepool(1000, 0.1f);
//
//	return tbml::ga::IGenepoolPtr(pool);
//}

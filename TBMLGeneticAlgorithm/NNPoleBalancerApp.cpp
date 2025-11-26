#include "stdafx.h"

//#include "NNPoleBalancerApp.h"
//#include "NNPoleBalancerAgent.h"
//#include "CommonImpl.h"
//
//tbml::ga::IGenepoolPtr NNPoleBalancerApp::createGenepool()
//{
//	auto pool = new tbml::ga::Genepool<NNGenome, NNPoleBalancerAgent>(
//		[]()
//	{
//		return std::make_shared<NNGenome>(tbml::nn::NeuralNetwork({
//			std::make_shared<tbml::nn::Layer::Dense>(4, 1),
//			std::make_shared<tbml::nn::Layer::TanH>() }));
//	},
//		[](std::shared_ptr<const NNGenome> genome)
//	{
//		return std::make_unique<NNPoleBalancerAgent>(std::move(genome), 1.0f, 0.1f, 1.0f, 1.0f, 2.5f, 4.0f, 30.0f);
//	});
//
//	pool->configThreading(false, true, false);
//	pool->resetGenepool(1000, 0.1f);
//
//	return tbml::ga::IGenepoolPtr(pool);
//}

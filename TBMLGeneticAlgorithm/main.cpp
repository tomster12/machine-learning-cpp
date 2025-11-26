#include "stdafx.h"
#include "App.h"
#include "VectorListTargetGenepool.h"

#define SCENARIO 3

int main()
{
	#if SCENARIO == 0
	NNTargetApp app;
	#elif SCENARIO == 1
	NNPoleBalancerApp app;
	#elif SCENARIO == 2
	NNDriverApp app;
	#elif SCENARIO == 3
	IAppGenepoolPtr genepool = VectorListTargetGenepool::createGenepool();
	#endif

	App app;
	return app.run(std::move(genepool));
}

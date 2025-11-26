#include "stdafx.h"
#include "NNTargetApp.h"
#include "NNPoleBalancerApp.h"
#include "NNDriverApp.h"
#include "VectorListTargetApp.h"

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
	VectorListTargetApp app;
	#endif

	return app.run();
}

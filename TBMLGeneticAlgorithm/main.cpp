#include "stdafx.h"
#include "NNTargetApp.h"
#include "NNPoleBalancerApp.h"
#include "NNDriverApp.h"

#define SCENARIO 2

int main()
{
	#if SCENARIO == 0
	NNTargetApp app;
	#elif SCENARIO == 1
	NNPoleBalancerApp app;
	#elif SCENARIO == 2
	NNDriverApp app;
	#endif

	return app.run();
}

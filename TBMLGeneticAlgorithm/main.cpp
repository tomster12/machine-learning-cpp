#include "stdafx.h"
#include "NNTargetApp.h"

int main()
{
	if (global::initialize() != 0) return 1;
	NNTargetApp app;
	return app.run();
}

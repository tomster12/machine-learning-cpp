#pragma once

#include "App.h"

class NNTargetApp : public App
{
protected:
	tbml::ga::IGenepoolPtr createGenepool() override;
};

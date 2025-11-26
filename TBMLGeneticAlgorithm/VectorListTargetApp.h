#pragma once

#include "App.h"

class VectorListTargetApp : public App
{
protected:
	tbml::ga::IGenepoolPtr createGenepool() override;
};

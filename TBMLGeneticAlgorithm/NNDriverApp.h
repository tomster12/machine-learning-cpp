#pragma once

#include <vector>
#include "App.h"

class NNDriverApp : public App
{
protected:
	tbml::ga::IGenepoolPtr createGenepool() override;
};

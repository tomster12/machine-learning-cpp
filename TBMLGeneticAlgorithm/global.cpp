#include "global.h"
#include "stdafx.h"

namespace global
{
	sf::Font font;

	int initialize()
	{
		font = sf::Font("assets/arial.ttf");
		return 0;
	}
}

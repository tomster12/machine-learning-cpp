#include "global.h"
#include "stdafx.h"

namespace global
{
	sf::Font font;

	void initialize()
	{
		font = sf::Font("assets/arial.ttf");
	}

	std::string formatFloat(float value, int decimals)
	{
		std::ostringstream ss;
		ss.setf(std::ios::fixed);
		ss.precision(decimals);
		ss << value;
		return ss.str();
	}
}

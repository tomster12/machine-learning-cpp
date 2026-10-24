#pragma once

namespace global
{
	extern sf::Font font;

	void initialize();
	std::string formatFloat(float value, int decimals = 2);
};

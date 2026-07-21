#pragma once

class ScreenVideoTest : public Screen
{
	double clock;
	Sprite sprite;
public:
	explicit ScreenVideoTest(GameWindow& window);

	bool Run(double dt);
};

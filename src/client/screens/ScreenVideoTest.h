#pragma once

class ScreenVideoTest : public Screen
{
	double clock;
	Sprite sprite;
public:
	ScreenVideoTest();

	bool Run(double dt);
};
#ifndef SCREEN_LD_H_
#define SCREEN_LD_H_

#include "Screen.h"
#include "SceneManager.h"
#include <boost/thread/thread.hpp>

class ScreenLoading : public Screen
{
	SceneManager* Animation;
	boost::thread *LoadThread;
public:
	ScreenLoading(Screen *Parent, Screen *_Next);
	void Init();

	bool RunIntro(float Fraction);
	bool RunExit(float Fraction);

	bool Run(double TimeDelta);
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(double xOff, double yOff);
	void Cleanup();
};

#endif
#ifndef SCREEN_LD_H_
#define SCREEN_LD_H_

#include "Screen.h"
#include "SceneEnvironment.h"
#include <boost/thread/thread.hpp>

class ScreenLoading : public Screen
{
	boost::thread *LoadThread;
public:
	ScreenLoading(Screen *Parent, Screen *_Next);
	void Init();

	void OnExitEnd();

	bool Run(double TimeDelta);
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(double xOff, double yOff);
	void Cleanup();
};

#endif
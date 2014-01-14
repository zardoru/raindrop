#ifndef SCREEN_LD_H_
#define SCREEN_LD_H_

#include "Screen.h"
#include "GraphObjectMan.h"
#include <boost/thread/thread.hpp>

class ScreenLoading : public IScreen
{
	GraphObjectMan Animation;
	boost::thread *LoadThread;
public:
	ScreenLoading(IScreen *Parent, IScreen *_Next);
	void Init();
	bool Run(double TimeDelta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void HandleScrollInput(double xOff, double yOff);
	void Cleanup();
};

#endif
#ifndef SCREEN_LD_H_
#define SCREEN_LD_H_

#include "Screen.h"
#include <boost/thread.hpp>

class ScreenLoading : public IScreen
{
	GraphObject2D mLogo;
	boost::thread *LoadThread;
public:
	ScreenLoading(IScreen *Parent, IScreen *_Next);
	void Init();
	bool Run(double TimeDelta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void Cleanup();
};

#endif
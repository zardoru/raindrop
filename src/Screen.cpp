#include "Global.h"
#include "Screen.h"

Screen::Screen() 
{  
	Parent = NULL;
	Running = false;
	Next = 0;
	ScreenTime = 0;
}

Screen::Screen(Screen *_Parent)
{
	Parent = _Parent;
	Running = false;
	Next = 0;
	ScreenTime = 0;
}

Screen::~Screen() {}

void Screen::LoadThreadInitialization()
{
	// virtual
}

void Screen::MainThreadInitialization()
{
	// virtual
}

bool Screen::IsScreenRunning()
{
	return Running;
}

bool Screen::RunNested(float delta)
{
	if (!Next)
		return false;

	if (Next->Run(delta))
		return true;
	else // The screen's done?
	{
		// It's not null- so we'll delete it.
		Next->Cleanup();
		delete Next;
		Next = 0;
		return false;
	}

	// Reaching this point SHOULDn't happen.
	return false;
}

void Screen::HandleInput(int32 key, KeyEventType state, bool isMouseInput) { /* stub */ }

void Screen::HandleScrollInput(double xOff, double yOff) { /* stub */ }

void Screen::Cleanup() { /* stub */ }

void Screen::Invalidate() { /* stub */ }
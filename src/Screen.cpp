#include "Global.h"
#include "Screen.h"

IScreen::IScreen() 
{  
	Parent = NULL;
	Running = false;
	Next = 0;
	ScreenTime = 0;
}

IScreen::IScreen(IScreen *_Parent)
{
	Parent = _Parent;
	Running = false;
	Next = 0;
	ScreenTime = 0;
}

IScreen::~IScreen() {}

void IScreen::LoadThreadInitialization()
{
	// virtual
}

void IScreen::MainThreadInitialization()
{
	// virtual
}

bool IScreen::IsScreenRunning()
{
	return Running;
}

bool IScreen::RunNested(float delta)
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

void IScreen::HandleInput(int32 key, KeyEventType state, bool isMouseInput) { /* stub */ }

void IScreen::Cleanup() { /* stub */ }

void IScreen::Invalidate() { /* stub */ }
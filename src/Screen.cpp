#include "Screen.h"

IScreen::IScreen() 
{  
	Running = false;
	Next = 0;
}

IScreen::IScreen(IScreen *_Parent)
{
	Parent = _Parent;
	Running = false;
	Next = 0;
}

IScreen::~IScreen() {}

void IScreen::Init()
{
	// pure virtual
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

void IScreen::Cleanup() { /* stub */ }

// The idea here is using nested screens for stuff such as battles after the overworld, all they'd need to do is put something like
// "This screen is done" then the battle scene gets destroyed, and we go back to the overworld scene. 
// There's a few drawbacks, it's going to consume a fair amount of memory, but other approaches
// would be a bit too complex- and it's not like we're going to allocate over a few dozen MB per-screen.
// Basically, for our requirements, this is the easiest to implement method. Though, if we wanted to change the way
// we run screens, we'd come back to this one and change it so it doesn't quite do nesting.
// It's powerful and very simple.
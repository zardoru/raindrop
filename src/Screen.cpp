#include "Global.h"
#include "Screen.h"

Screen::Screen() 
{  
	Parent = NULL;
	Running = false;
	Next = 0;
	ScreenTime = 0;
	IntroDuration = 0;
	ScreenState = StateRunning;
}

Screen::Screen(Screen *_Parent)
{
	Parent = _Parent;
	Running = false;
	Next = 0;
	ScreenTime = 0;
	IntroDuration = 0;
	ScreenState = StateRunning;
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

void Screen::ChangeState(Screen::EScreenState NewState)
{
	ScreenState = NewState;
	TransitionTime = 0;
}

bool Screen::IsScreenRunning()
{
	return Running;
}

bool Screen::RunNested(float delta)
{
	if (!Next)
		return false;

	if (Next->Update(delta))
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

Screen* Screen::GetTop()
{
	if (Next) return Next->GetTop();
	else return this;
}

double Screen::GetScreenTime()
{
	return ScreenTime;
}

bool Screen::Update(float delta)
{
	ScreenTime += delta;

	if (ScreenState == StateIntro)
	{
		float Frac;
		TransitionTime += delta;

		if (TransitionTime < IntroDuration)
			Frac = Clamp(TransitionTime / IntroDuration, 0.0, 1.0);
		else
		{
			Frac = 1;
			ScreenState = StateRunning;
		}

		return RunIntro(Frac);
	}else if (ScreenState == StateExit)
	{
		float Frac;

		TransitionTime += delta;
		if (TransitionTime < ExitDuration)
			Frac = Clamp(TransitionTime / ExitDuration, 0.0, 1.0);
		else
		{
			Frac = 1;
			ScreenState = StateRunning;
		}

		/* 
			StateExit can still go back to the "StateRunning" state
			This way it can be used for transitions.
		*/
		return RunExit(Frac); 
	}else
		return Run(delta);
}

bool Screen::RunIntro(float Fraction) { return true; }

bool Screen::RunExit(float Fraction) {	return false; }

void Screen::HandleInput(int32 key, KeyEventType state, bool isMouseInput) { /* stub */ }

void Screen::HandleScrollInput(double xOff, double yOff) { /* stub */ }

void Screen::Cleanup() { /* stub */ }

void Screen::Invalidate() { /* stub */ }
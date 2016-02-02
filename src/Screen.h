#pragma once

#include "Interruptible.h"
class SceneEnvironment;

// Interface.
class Screen : public Interruptible
{
private:
	double ScreenTime; // How long has it been open?
protected:

    std::shared_ptr<SceneEnvironment> Animations;

	enum EScreenState {
		StateIntro,
		StateRunning,
		StateExit
	}ScreenState;

	double GetScreenTime();
    std::shared_ptr<Screen> Parent;
	bool Running; // Is this screen active?
	bool SkipThisFrame;

	void ChangeState(EScreenState NewState);
	double TransitionTime;
	double IntroDuration, ExitDuration;
    std::shared_ptr<Screen> Next;

public:
	Screen (GString Name);
	Screen (GString Name, std::shared_ptr<Screen> _Parent);
	virtual ~Screen ();

	virtual void Init();

	// Nesting screens.
	bool IsScreenRunning();
	bool RunNested(float delta);
	bool Update(float delta);

	void Close();

	Screen* GetTop();

	// Screen implementation.
	virtual void LoadResources(); // could, or not, be called from main thread.
	virtual void InitializeResources(); // must be called from main thread - assume it always is
	virtual bool RunIntro(float Fraction, float Delta);
	virtual bool RunExit(float Fraction, float Delta);
	virtual bool Run(double delta) = 0;

	virtual void OnIntroBegin();
	virtual void OnIntroEnd();
	virtual void OnRunningBegin();
	virtual void OnExitBegin();
	virtual void OnExitEnd();

	virtual bool HandleInput(int32 key, KeyEventType state, bool isMouseInput);
	virtual bool HandleScrollInput(double xOff, double yOff);
	virtual bool HandleTextInput(int codepoint);

	// We need to set up graphics again? This gets called.
	virtual void Invalidate();

	// Implement this if there's anything you want to get done outside of a destructor
	// like operations that would throw exceptions.
	virtual void Cleanup();

};
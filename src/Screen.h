#ifndef ISCREEN_H_
#define ISCREEN_H_

// Interface.
class Screen
{
private:
	double ScreenTime; // How long has it been open?
protected:

	enum EScreenState {
		StateIntro,
		StateRunning,
		StateExit
	}ScreenState;

	double GetScreenTime();
	Screen *Parent;
	bool Running; // Is this screen active?

	void ChangeState(EScreenState NewState);
	double TransitionTime;
	double IntroDuration, ExitDuration;

public:

	Screen *Next;

	Screen ();
	Screen (Screen * _Parent);
	virtual ~Screen ();

	// Nesting screens.
	bool IsScreenRunning();
	bool RunNested(float delta);
	bool Update(float delta);
	void Close();

	Screen* GetTop();

	// Screen implementation.
	virtual void LoadThreadInitialization();
	virtual void MainThreadInitialization();
	virtual bool RunIntro(float Fraction);
	virtual bool RunExit(float Fraction);
	virtual bool Run(double delta) = 0;
	virtual bool HandleInput(int32 key, KeyEventType state, bool isMouseInput);
	virtual bool HandleScrollInput(double xOff, double yOff);
	virtual bool HandleTextInput(int codepoint);

	// We need to set up graphics again? This gets called.
	virtual void Invalidate();

	// Implement this if there's anything you want to get done outside of a destructor
	// like operations that would throw exceptions.
	virtual void Cleanup();

};

#endif
#ifndef ISCREEN_H_
#define ISCREEN_H_

class SceneEnvironment;

// Interface.
class Screen
{
private:
	double ScreenTime; // How long has it been open?
protected:

	shared_ptr<SceneEnvironment> Animations;

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
	Screen *Next;

public:
	Screen (GString Name);
	Screen (GString Name, Screen * _Parent);
	virtual ~Screen ();

	void Init();

	// Nesting screens.
	bool IsScreenRunning();
	bool RunNested(float delta);
	bool Update(float delta);
	void Close();

	Screen* GetTop();

	// Screen implementation.
	virtual void LoadThreadInitialization();
	virtual void MainThreadInitialization();
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

#endif
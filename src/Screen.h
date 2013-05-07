#ifndef ISCREEN_H_
#define ISCREEN_H_

// Interface.
class IScreen
{
protected:
	IScreen *Parent;
	float ScreenTime; // How long has it been open?
	bool Running; // Is this screen active?

public:

	IScreen *Next;

	IScreen ();
	IScreen (IScreen * _Parent);
	virtual ~IScreen ();

	// Nesting screens.
	bool IsScreenRunning();
	bool RunNested(float delta);

	// Screen implementation.
	virtual void Init();
	virtual bool Run(double delta) = 0;
	virtual void HandleInput(int32 key, int32 state, bool isMouseInput) = 0;

	// Implement this if there's anything you want to get done outside of a destructor
	// like operations that would throw exceptions.
	virtual void Cleanup();

};

#endif
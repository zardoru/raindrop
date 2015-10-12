#ifndef SCREEN_LD_H_
#define SCREEN_LD_H_

class ScreenLoading : public Screen
{
	thread *LoadThread;
	bool ThreadInterrupted;
	atomic<bool> FinishedLoading;
public:
	ScreenLoading(Screen *Parent, Screen *_Next);
	void Init() override;

	void OnIntroBegin() override;
	void OnExitEnd() override;

	bool Run(double TimeDelta) override;
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput) override;
	bool HandleScrollInput(double xOff, double yOff) override;
	void Cleanup();
};

#endif
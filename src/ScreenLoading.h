#pragma once

class ScreenLoading : public Screen
{
	shared_ptr<thread> LoadThread;
	bool ThreadInterrupted;
	atomic<bool> FinishedLoading;
public:
	ScreenLoading(shared_ptr<Screen> _Next);
	void Init() override;

	void OnIntroBegin() override;
	void OnExitEnd() override;

	bool Run(double TimeDelta) override;
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput) override;
	bool HandleScrollInput(double xOff, double yOff) override;
	void Cleanup();
};
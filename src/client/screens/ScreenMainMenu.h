#pragma once


class SceneEnvironment;

class ScreenMainMenu : public Screen
{
    Screen *TNext;
public:
    ScreenMainMenu();
    void Init();
    bool HandleInput(int32_t key, bool isPressed, bool isMouseInput);
    bool HandleScrollInput(double xOff, double yOff);

    void OnExitEnd();

    bool Run(double Delta);
    void Cleanup();
};
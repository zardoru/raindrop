#pragma once


class SceneEnvironment;

class ScreenMainMenu : public Screen
{
    Screen *TNext;
public:
    ScreenMainMenu();
    void Init();
    bool on_input(int32_t key, bool isPressed, bool isMouseInput);
    bool on_scroll_input(double xOff, double yOff);

    void OnExitEnd();

    bool Run(double Delta);
    void cleanup();
};
#pragma once


class SceneEnvironment;

class ScreenMainMenu : public Screen
{
    Screen *TNext;
public:
    explicit ScreenMainMenu(GameWindow& window);
    void init();
    bool on_input(int32_t key, bool isPressed, bool isMouseInput);
    bool on_scroll_input(double xOff, double yOff);

    void on_exit_end();

    bool run(double Delta);
    void cleanup();
};

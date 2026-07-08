#pragma once

class ScreenLoading : public Screen
{
    std::shared_ptr<std::thread> LoadThread;
    bool ThreadInterrupted;
    std::atomic<bool> FinishedLoading;
public:
    ScreenLoading(std::shared_ptr<Screen> _Next);
    void init() override;

    void on_intro_begin() override;
    void on_exit_end() override;

    bool run(double TimeDelta) override;
    bool on_input(int32_t key, bool isPressed, bool isMouseInput) override;
    bool on_scroll_input(double xOff, double yOff) override;
    void cleanup() override;
};
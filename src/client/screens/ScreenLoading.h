#pragma once

class ScreenLoading : public Screen
{
    std::shared_ptr<std::thread> load_thread_;
    bool thread_interrupted_;
    std::atomic<bool> finished_loading_;
public:
    ScreenLoading(GameWindow& window, std::shared_ptr<Screen> next);
    void init() override;

    void on_intro_begin() override;
    void on_exit_end() override;

    bool run(double time_delta) override;
    bool on_input(int32_t key, bool isPressed, bool isMouseInput) override;
    bool on_scroll_input(double xOff, double yOff) override;
    void cleanup() override;
};

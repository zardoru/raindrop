#pragma once

class ScreenCustom : public Screen
{
public:
    ScreenCustom(GameWindow& window, const std::filesystem::path& script_name);
    bool run(double Delta) override;
    bool on_input(int32_t key, bool isPressed, bool isMouseInput) override;
};

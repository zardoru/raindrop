#pragma once

class ScreenCustom : public Screen
{
public:
    ScreenCustom(const std::filesystem::path& ScriptName);
    bool Run(double Delta) override;
    bool on_input(int32_t key, bool isPressed, bool isMouseInput) override;
};
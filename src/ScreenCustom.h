#pragma once

#include "Screen.h"

class ScreenCustom : public Screen
{
public:
    ScreenCustom(const std::string& ScriptName);
    bool Run(double Delta) override;
    bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput) override;
};
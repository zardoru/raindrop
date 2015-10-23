
#pragma once

#include "Screen.h"

class ScreenCustom : public Screen
{
public:
	ScreenCustom(const GString& ScriptName);
	bool Run(double Delta) override;
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput) override;
};
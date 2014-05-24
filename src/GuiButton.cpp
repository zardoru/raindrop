#include "GameGlobal.h"
#include "GameWindow.h"
#include "GuiButton.h"

using namespace GUI;

Button::Button()
{
	OnHover = OnClick = OnLeave = NULL;
	PressedDown = false;
}

bool Button::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (isMouseInput && BindingsManager::TranslateKey(key) == KT_Select)
	{
		Vec2 mpos = WindowFrame.GetRelativeMPos();
		Vec2 bpos = GetPosition();
		if (mpos.x > bpos.x && mpos.x < bpos.x+GetWidth())
		{
			if (mpos.y > bpos.y && mpos.y < bpos.y+GetHeight())
			{
				if (code == KE_Press)
				{
					if (OnClick) OnClick;
					PressedDown = true;
				}
			}
		}

		if (code == KE_Release && PressedDown)
		{
			if (OnRelease) OnRelease;
			PressedDown = false;
			return true;
		}
	}

	return false;
}
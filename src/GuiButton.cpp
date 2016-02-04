#include "pch.h"

#include "GameGlobal.h"
#include "GameWindow.h"
#include "GuiButton.h"

using namespace GUI;

Button::Button()
{
    PressedDown = false;
    Hovering = false;
}

bool MouseInBoundaries(const Mat4 &Inverse, bool Centered)
{
    Vec2 mpos = WindowFrame.GetRelativeMPos();
    glm::vec4 mv4(mpos.x, mpos.y, 0, 1);
    glm::vec4 screenPos = (mv4 * Inverse);

    if (Centered)
    {
        if (screenPos.x <= 0.5 && screenPos.x >= -0.5) // Within X boundaries
        {
            if (screenPos.y <= 0.5 && screenPos.y >= -0.5) // Within Y boundaries
            {
                return true;
            }
            else return false;
        }
        else return false;
    }
    else
    {
        if (screenPos.x <= 1 && screenPos.x >= 0) // Within X boundaries
        {
            if (screenPos.y <= 1 && screenPos.y >= 0) // Within Y boundaries
            {
                return true;
            }return false;
        }return false;
    }
}

bool Button::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
{
    if (isMouseInput && BindingsManager::TranslateKey(key) == KT_Select)
    {
        if (MouseInBoundaries(transformReverse, Centered) && code == KE_PRESS)
        {
            if (OnClick) OnClick(this);
            PressedDown = true;
        }

        if (code == KE_RELEASE && PressedDown)
        {
            if (OnRelease) OnRelease(this);
            PressedDown = false;
            return true;
        }
    }

    return false;
}

void Button::HoverUpdate(bool IsHovering)
{
    bool OldHoveringState = Hovering;
    Hovering = IsHovering;

    if (Hovering != OldHoveringState) // We've changed state (false => true, true => false)
    {
        if (Hovering) // state is true, we've entered hovering
        {
            if (OnHover) OnHover(this);
        }
        else
            if (OnLeave) OnLeave(this);
    }
}

void Button::Run(double delta)
{
    transformReverse = GetMatrix()._inverse();

    if (MouseInBoundaries(transformReverse, Centered))
        HoverUpdate(true);
    else HoverUpdate(false);
}
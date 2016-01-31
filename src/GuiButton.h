#pragma once

#include "Sprite.h"

typedef function<void(Sprite*)> EventAnimationFunction;

namespace GUI
{

class Button : public Sprite
{
	Mat4 transformReverse;

	bool PressedDown;
	bool Hovering;

	void HoverUpdate(bool IsHovering);

public:

	EventAnimationFunction OnHover;
	EventAnimationFunction OnLeave;
	EventAnimationFunction OnClick;
	EventAnimationFunction OnRelease;

	Button();
	// True if clicked!
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void Run(double delta); // We assume this is called BEFORE Render() is.
};

}
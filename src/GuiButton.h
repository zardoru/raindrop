#ifndef GUI_BUTTON_H_
#define GUI_BUTTON_H_

#include "GraphObject2D.h"

typedef void(*EventAnimationFunction)(GraphObject2D*);

namespace GUI
{

class Button : public GraphObject2D
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

#endif
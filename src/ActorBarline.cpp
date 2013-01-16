#include "Global.h"
#include "GraphObject2D.h"
#include "ScreenGameplay.h"
#include "ActorBarline.h"
#include "GraphicsManager.h"
#include "Game_Consts.h"
#include <GL/glfw.h>

ActorBarline::ActorBarline(ScreenGameplay *_Parent) : GraphObject2D()
{
	Parent = _Parent;
	origin = 1; //use center
}

void ActorBarline::Init(float Offset)
{
	AnimationProgress = AnimationTime = Offset;

	position.x = Parent->GetScreenOffset(0.5).x;
	position.y = height / 2 + ScreenOffset + 3 * PlayfieldWidth / 4;
	width = PlayfieldWidth;

	red = green = 0;
	blue = 200.f / 255.f;

	GraphObject2D::Init(true);
}

void ActorBarline::Run(float TimeDelta, float MeasureTime, float TotalTime)
{
	if (AnimationProgress > 0)
	{
		float PosY = pow(AnimationProgress / AnimationTime, 2) * PlayfieldHeight + ScreenOffset + height / 2;

		alpha = 1 - AnimationProgress;

		position.y = PosY;
		AnimationProgress -= TimeDelta;

		if (AnimationProgress <= 0)
			AnimationProgress = 0;
		
		return; 
	}
	
	if (Parent->GetMeasure() % 2)
	{
		position.y = PlayfieldHeight + ScreenOffset - PlayfieldHeight * TotalTime / MeasureTime + height / 2;
		red = 1;
		blue = green = 0;
	}else
	{
		position.y = PlayfieldHeight * TotalTime / MeasureTime + height / 2 + ScreenOffset;
		red = 0.0;
		blue = 200.f / 255.f;
	}
}
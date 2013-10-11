#include "Global.h"
#include "GraphObject2D.h"
#include "ScreenGameplay.h"
#include "ActorBarline.h"
#include "GraphicsManager.h"
#include "Game_Consts.h"

ActorBarline::ActorBarline(ScreenGameplay *_Parent) : GraphObject2D()
{
	Parent = _Parent;
	Centered = true;
	ColorInvert = false;
}

void ActorBarline::Init(float Offset)
{
	AnimationProgress = AnimationTime = Offset;

	position.x = Parent->GetScreenOffset(0.5).x;
	position.y = ScreenOffset + 3 * PlayfieldWidth / 4;
	width = PlayfieldWidth;

	red = green = 0;
	blue = 200.f / 255.f;
	alpha = 0;
	GraphObject2D::InitTexture();
}

#define RadioThreshold 0.9

void ActorBarline::Run(double TimeDelta, double MeasureTime, double TotalTime)
{
	float Ratio = TotalTime / MeasureTime;
	if (AnimationProgress > 0)
	{
		float PosY = pow(AnimationProgress / AnimationTime, 2) * PlayfieldHeight + ScreenOffset;

		alpha = 1 - AnimationProgress;

		position.y = PosY;
		AnimationProgress -= TimeDelta;

		if (AnimationProgress <= 0)
			AnimationProgress = 0;
		
		return; 
	}
	
	if (Parent->GetMeasure() % 2)
	{
		if (Ratio > RadioThreshold)
		{
			float diff = Ratio - RadioThreshold;
			float duration = (1 - RadioThreshold);

			red = 1 - ( diff/duration );
			blue = (200.f / 255.f) * (diff/duration);
		}
		else red = 1;
		blue = green = 0;
	}else
	{
		red = 0.0;
		blue = 200.f / 255.f;

		if (Ratio > RadioThreshold)
		{
			float diff = Ratio - RadioThreshold;
			float duration = (1 - RadioThreshold);

			red = 1 * (diff/duration);
			blue = 200.f / 255.f - (200.f / 255.f) * (diff/duration);
		}
	}

	position.y = (Ratio) * (float)PlayfieldHeight;
			
	if (Parent->GetMeasure() % 2)
		position.y = PlayfieldHeight - position.y;

	position.y += ScreenOffset;
}
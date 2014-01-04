#include "Global.h"
#include "GraphObject2D.h"
#include "ScreenGameplay.h"
#include "ActorBarline.h"
#include "GameWindow.h"
#include "Game_Consts.h"

ActorBarline::ActorBarline(ScreenGameplay *_Parent) : GraphObject2D()
{
	Parent = _Parent;
	Centered = true;
	ColorInvert = false;
	AnimationTime = 0;
	AnimationProgress = 0;
	AffectedByLightning = true;
}

void ActorBarline::Init(float Offset)
{
	AnimationProgress = AnimationTime = Offset;

	SetPosition(Parent->GetScreenOffset(0.5).x, ScreenOffset + 3 * PlayfieldWidth / 4);
	SetWidth(PlayfieldWidth);

	Red = Green = 0;
	Blue = 200.f / 255.f;
	Alpha = 0;
}

#define RadioThreshold 0.9

void ActorBarline::Run(double TimeDelta, double Ratio)
{
	if (AnimationProgress > 0)
	{
		float PosY = pow(AnimationProgress / AnimationTime, 2) * PlayfieldHeight + ScreenOffset;

		Alpha = 1 - AnimationProgress;

		SetPositionY(PosY);
		AnimationProgress -= TimeDelta;

		if (AnimationProgress <= 0)
			AnimationProgress = 0;
		
		return; 
	}
	
	if (Parent->GetMeasure() % 2)
	{
		Red = 1;
		Blue = Green = 0;

		if (Ratio > RadioThreshold)
		{
			double diff = Ratio - RadioThreshold;
			double duration = (1 - RadioThreshold);

			Red = 1 - ( diff/duration );
			Blue = (200.f / 255.f) * (diff/duration);
		}
	}else
	{
		Red = 0.0;
		Blue = 200.f / 255.f;

		if (Ratio > RadioThreshold)
		{
			float diff = Ratio - RadioThreshold;
			double duration = (1 - RadioThreshold);

			Red = 1 * (diff/duration);
			Blue = 200.f / 255.f - (200.f / 255.f) * (diff/duration);
		}
	}

	SetPositionY(Ratio * (float)PlayfieldHeight);
			
	if (Parent->GetMeasure() % 2)
		SetPositionY(PlayfieldHeight - GetPosition().y);

	SetPositionY(GetPosition().y + ScreenOffset);

	// assert(GetPosition().y > ScreenOffset);
}
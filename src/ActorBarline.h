#pragma once

class ScreenGameplay;

class ActorBarline : public Sprite
{
	ScreenGameplay* Parent;
	float AnimationTime, AnimationProgress;
public:
	ActorBarline(ScreenGameplay *_Parent);
	void Run(double TimeDelta, double Ratio);
	void Init(float Offset);
};
#ifndef ACTOR_BARLINE_H_
#define ACTOR_BARLINE_H_

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

#endif
#ifndef ACTOR_BARLINE_H_
#define ACTOR_BARLINE_H_

class ScreenGameplay;

class ActorBarline : public GraphObject2D
{
	ScreenGameplay* Parent;
	float AnimationTime, AnimationProgress;
public:
	ActorBarline(ScreenGameplay *_Parent);
	void Run(float TimeDelta, float MeasureTime, float TotalTime);
	void Init(float Offset);
};

#endif
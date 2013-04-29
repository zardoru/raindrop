#ifndef ACTOR_BARLINE_H_
#define ACTOR_BARLINE_H_

class ScreenGameplay;

class ActorBarline : public GraphObject2D
{
	ScreenGameplay* Parent;
	float AnimationTime, AnimationProgress;
public:
	ActorBarline(ScreenGameplay *_Parent);
	void Run(double TimeDelta, double MeasureTime, double TotalTime);
	void Init(float Offset);
};

#endif
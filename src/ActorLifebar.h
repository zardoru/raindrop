#ifndef ACTOR_LIFEBAR_H_
#define ACTOR_LIFEBAR_H_

class ActorLifebar : public GraphObject2D
{
	float pending_health, time;
public:
	// I can't find anything more obvious of a name.
	ActorLifebar();
	float Health;
	void HitJudgement(Judgement Hit);
	void Run (double delta);
	void UpdateHealth();
};

#endif

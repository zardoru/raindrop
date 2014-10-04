#ifndef ACTOR_JUDGEMENT_H_
#define ACTOR_JUDGEMENT_H_

class ActorJudgment : public GraphObject2D
{
	float AnimTime;
public:
	// I can't find anything more obvious of a name.
	ActorJudgment();
	void ChangeJudgment(Judgment New);
	void Run (double delta);
};

#endif
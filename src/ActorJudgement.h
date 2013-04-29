#ifndef ACTOR_JUDGEMENT_H_
#define ACTOR_JUDGEMENT_H_

class ActorJudgement : public GraphObject2D
{
	float AnimTime;
public:
	// I can't find anything more obvious of a name.
	ActorJudgement();
	void ChangeJudgement(Judgement New);
	void Run (double delta);
};

#endif
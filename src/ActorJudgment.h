#pragma once

class ActorJudgment : public Sprite
{
    float AnimTime;
public:
    // I can't find anything more obvious of a name.
    ActorJudgment();
    void ChangeJudgment(Judgment New);
    void Run(double delta);
};
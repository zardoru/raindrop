#pragma once

class ActorLifebar : public Sprite
{
    float pending_health, time;
public:
    // I can't find anything more obvious of a name.
    ActorLifebar();
    float Health;
    void HitJudgment(Judgment Hit);
    void Run(double delta);
    void UpdateHealth();
};

#pragma once

#include "Interruptible.h"
#include "Sprite.h"

namespace Game
{
    class Song;
}

class BackgroundAnimation : public Interruptible
{
protected:
    Transformation Transform;
public:
    BackgroundAnimation(Interruptible* parent = nullptr);
    virtual ~BackgroundAnimation() = default;
    virtual void SetAnimationTime(double Time);
    virtual void Load();
    virtual void Validate();
    virtual void Update(float Delta);

    virtual void OnHit();
    virtual void OnMiss();
    virtual void Render();

    Transformation& GetTransformation();
    /* Can only be called from main thread! */
    static std::shared_ptr<BackgroundAnimation> CreateBGAFromSong(uint8_t DifficultyIndex, Game::Song& Input, Interruptible* context, bool LoadNow = false);
};
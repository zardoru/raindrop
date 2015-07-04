#pragma once

#include "Sprite.h"

namespace Game{
	class Song;
}

class BackgroundAnimation : public Drawable2D
{
public:
	virtual ~BackgroundAnimation() = default;
	virtual void SetAnimationTime(double Time);
	virtual void Load();
	virtual void Validate();
	virtual void Update(float Delta);

	virtual void OnHit();
	virtual void OnMiss();

	/* Can only be called from main thread! */
	static shared_ptr<BackgroundAnimation> CreateBGAFromSong(uint8_t DifficultyIndex, Game::Song& Input, bool LoadNow = false);
};
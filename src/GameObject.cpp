#include "Global.h"
#include "GraphObject2D.h"
#include "GameObject.h"
#include "ImageLoader.h"
#include "FileManager.h"
#include "Audio.h"

SoundSample *HitSnd = NULL;
SoundSample *HoldReleaseSnd = NULL;
bool GameObjectTexInitialized = false;
uint32 GameObjectUVvbo;

GameObject::GameObject() : GraphObject2D(false)
{
	SetImage(ImageLoader::LoadSkin("hitcircle.png"));
	Centered = true; // use the object's center instead of top-left
	
	SetSize(CircleSize);

	fadeout_time = 0;
	fadein_time = 0.7f;
	hold_duration = 0;
	endTime = 0;
	heldKey = -1;
	BeingHeld = false;
	DoTextureCleanup = false;

	if (!HitSnd)
	{
		HitSnd = new SoundSample((FileManager::GetSkinPrefix() + "/hit.ogg").c_str());
		MixerAddSample(HitSnd);
	}

	if (!HoldReleaseSnd)
	{
		HoldReleaseSnd  = new SoundSample((FileManager::GetSkinPrefix() + "/holdfinish.ogg").c_str());
		MixerAddSample(HoldReleaseSnd);
	}

#ifndef OLD_GL
	if (GameObjectTexInitialized)
		ourUVBuffer = GameObjectUVvbo;
	else
	{
		InitTexture();
		GameObjectUVvbo = ourUVBuffer;
		GameObjectTexInitialized = true;
	}
#endif
	ColorInvert = false;
}

void GameObject::Animate(float delta, float songTime)
{
	if (GetPosition().x == 0)
	{
		Alpha = 0;
		return;
	}

	if (fadeout_time > 0)
	{
		fadeout_time -= delta*2;

		// alpha out
		Alpha = 1 * (fadeout_time);

		if (Alpha < 0)
			Alpha = 0;

		// scale in
		if (endTime == 0)
		{
			SetScale(2 - fadeout_time);
		}
		else
			SetScale(3 - fadeout_time);

		return;
	}

	if (fadein_time > 0 && !fadeout_time)
	{
		Alpha = 1-fadein_time*2;
		fadein_time -= delta;
	}

	if (BeingHeld == true)
	{
		float holdDuration = endTime - startTime;
		float Progress = songTime - startTime;
		SetScale(1 + 0.3*Progress/holdDuration);
		Green = 0.5 - 0.5 * Progress/holdDuration;
	}
}

Judgement GameObject::Run(double delta, double Time, bool Autoplay)
{
	if (fadeout_time || GetPosition().x == 0) // It was hit, so stop.
		return None;

	if (Autoplay)
	{
		// you can be slightly early autoplay, it's fine.
		if (Time >= startTime - 0.001 && fadeout_time == 0 && !BeingHeld) // A pretend kind of thing. ;)
		{
			return Hit(Time, GetPosition(), true, false, -1);
		}
	}

	// it's not being held? Out of leniency?
	if (Time - startTime > LeniencyHitTime)
	{
		if (endTime == 0)
		{
			fadeout_time = 0.7f; // Fade out! You missed!
			return Miss;
		}
	}

	if (endTime == 0) // stuff forward is holds only
		return None;

	if (BeingHeld)
	{
		/* it's done */
		if (Time >= endTime && fadeout_time == 0)
		{
			fadeout_time = 1;
			BeingHeld = false;
			HoldReleaseSnd->Reset();
			return OK;
		}
	}else if (!BeingHeld && Time > startTime+LeniencyHitTime)
	{
		fadeout_time = 0.7f;
		return NG;
	}

	return None;
}

Judgement GameObject::Hit(float Time, glm::vec2 mpos, bool KeyDown,  bool Autoplay, int32 Key)
{
	glm::vec2 dist = mpos - GetPosition();
	
	if (fadeout_time || GetPosition().x == 0 || Autoplay)
		return None;

	if (BeingHeld) // Held note? Handle things here.
	{
		if (!KeyDown && Key == heldKey) // Same key we pressed initially? It was released! Panic!
			BeingHeld = false;

		return None;
	}

	if (!KeyDown)
	{
		if (!Autoplay)
			return None;
	}

	// not already fading out and mouse within the circle?
	float dabs = std::abs(glm::length(dist))*2; // dabs*2 < diameter; dabs < radius
	if (dabs < CircleSize && fadeout_time == 0) 
	{
		float SpareTime = std::abs(Time - startTime);
		if (SpareTime < LeniencyHitTime) // within leniency?
		{
			// Hit!
			Judgement RVal;

			HitSnd->Reset();

			if (endTime == 0) // Not a hold?
				fadeout_time = 0.6f; // 0.8 secs for fadeout
			else
			{
				heldKey = Key;
				BeingHeld = true;
			}


			// Judgement (values in seconds
			if (SpareTime < 0.35)
				RVal = Bad;

			if (SpareTime < 0.25)
				RVal = Great;

			if (SpareTime < 0.1)
				RVal = Perfect;

			if (SpareTime < 0.05)
				RVal = Excellent;

			return RVal;
		}
	}
	return None;
}

/* Assume GraphObject2D was already invalidated*/ 
void GameObject::Invalidate()
{
#ifndef OLD_GL
	InitTexture();
	GameObjectUVvbo = ourUVBuffer;
#endif
}
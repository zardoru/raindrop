#include "Global.h"
#include "GraphObject2D.h"
#include "VBO.h"
#include "GameObject.h"
#include "ImageLoader.h"
#include "FileManager.h"
#include "Audio.h"

SoundSample *HitSnd = NULL;
SoundSample *HoldReleaseSnd = NULL;
bool GameObjectTexInitialized = false;
VBO *GameObjectUVvbo = NULL;

GameObject::GameObject() : GraphObject2D(false)
{
	SetImage(ImageLoader::LoadSkin("hitcircle.png"));
	Centered = true; // use the object's center instead of top-left
	
	SetSize(CircleSize);

	fadeout_time = 0;
	fadein_time = 0.7f;
	AnimationStatus = 0;
	hold_duration = 0;
	endTime = 0;
	heldKey = -1;
	BeingHeld = false;
	DoTextureCleanup = false;
	waiting_time = 0;
	AffectedByLightning = true;

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

	if (GameObjectTexInitialized)
		UvBuffer = GameObjectUVvbo;
	else
	{
		UpdateTexture();
		GameObjectUVvbo = UvBuffer;
		GameObjectTexInitialized = true;
	}
	ColorInvert = false;
}

void GameObject::Animate(float delta, float songTime)
{
	if (GetPosition().x == 0)
	{
		Alpha = 0;
		return;
	}

	if (AnimationStatus == 0)
	{
		if (fadein_time >= 0)
		{
			Alpha = 0;
			if (waiting_time)
			{
				waiting_time -= delta;
			}

			if (waiting_time <= 0)
			{
				Alpha = 1-fadein_time*5;
				fadein_time -= delta;
			}

			return;
		}
		AnimationStatus = 1;
	}else if (AnimationStatus == 1)
	{
		Alpha = 1;
		
		if (fadeout_time)
			AnimationStatus = 2;
	}
	else
	{
		fadeout_time -= delta*2;

		// alpha out
		Alpha = 1 * (fadeout_time);

		if (Alpha <= 0)
		{
			Alpha = 0;
			AnimationStatus = 3; // Remove only
		}

		// scale in
		if (endTime == 0)
		{
			SetScale(2 - fadeout_time);
		}
		else
			SetScale(3 - fadeout_time);

		return;
	}

	

	if (BeingHeld == true)
	{
		float holdDuration = endTime - startTime;
		float Progress = songTime - startTime;
		SetScale(1 + 0.3*Progress/holdDuration);
		Green = 0.5 - 0.5 * Progress/holdDuration;
	}
}

bool GameObject::ShouldRemove()
{
	return AnimationStatus == 3;
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
		fadeout_time = 1;
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
			RVal = Bad;

			if (SpareTime < DotcurGreatLeniency)
				RVal = Great;

			if (SpareTime < DotcurPerfectLeniency)
				RVal = Perfect;

			if (SpareTime < DotcurExcellentLeniency)
				RVal = Excellent;

			return RVal;
		}
	}
	return None;
}

bool GameObject::IsHold()
{
	return endTime > 0;
}

void GameObject::Assign(float Duration, uint32 _Measure, uint32 _MeasureFraction)
{
	hold_duration = Duration;
	Measure = _Measure;
	MeasurePos = _MeasureFraction;
}

/* Assume GraphObject2D was already invalidated*/ 
void GameObject::Invalidate()
{
	UvBuffer->Invalidate();
	UpdateTexture();
	GameObjectUVvbo = UvBuffer;
}
#include "GameGlobal.h"
#include "GameState.h"
#include "Sprite.h"
#include "VBO.h"
#include "GameObject.h"
#include "ImageLoader.h"
#include "Audio.h"

#define FADEIN_DURATION 0.3f
#define FADEOUT_DURATION 0.7f

SoundSample *HitSnd = NULL;
SoundSample *HoldReleaseSnd = NULL;
bool GameObjectTexInitialized = false;

GameObject::GameObject() : Sprite(false)
{
	SetImage(GameState::GetInstance().GetSkinImage("hitcircle.png"));
	Centered = true; // use the object's center instead of top-left
	
	SetSize(CircleSize);

	fadeout_time = 0;
	fadein_time = FADEIN_DURATION;
	AnimationStatus = 0;
	hold_duration = 0;
	endTime = 0;
	heldKey = -1;
	Fraction = -1;
	BeingHeld = false;
	DoTextureCleanup = false;
	waiting_time = 0;
	AffectedByLightning = true;

	if (!HitSnd)
	{
		HitSnd = new SoundSample();
		HitSnd->Open((GameState::GetInstance().GetSkinFile("hit.ogg")).c_str());
		MixerAddSample(HitSnd);
	}

	if (!HoldReleaseSnd)
	{
		HoldReleaseSnd  = new SoundSample();
		HoldReleaseSnd->Open((GameState::GetInstance().GetSkinFile("holdfinish.ogg")).c_str());
		MixerAddSample(HoldReleaseSnd);
	}

	UvBuffer = TextureBuffer;

	ColorInvert = false;
}

void GameObject::GlobalInit()
{
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
				Alpha = LerpRatio(0.0, 1.0, FADEIN_DURATION - fadein_time, FADEIN_DURATION);
				SetScale ( LerpRatio(1.5, 1.0, FADEIN_DURATION - fadein_time, FADEIN_DURATION) );
				fadein_time -= delta;
			}

			return;
		}

		Alpha = 1;
		SetScale (1);
		AnimationStatus = 1;
	}else if (AnimationStatus == 1)
	{
		if (fadeout_time)
			AnimationStatus = 2;
	}
	else
	{
		fadeout_time -= delta*2;

		// alpha out
		Alpha = LerpRatio (1.0, 0.0, FADEOUT_DURATION - fadeout_time, FADEOUT_DURATION);

		if (fadeout_time <= 0)
		{
			Alpha = 0;
			AnimationStatus = 3; // Remove only
		}

		// scale in
		if (endTime == 0)
		{
			SetScale(LerpRatio(1.0, 2.0, FADEOUT_DURATION - fadeout_time, FADEOUT_DURATION));
		}
		else // Holds have a bigger scale-in
			SetScale(LerpRatio(1.3, 3.0, FADEOUT_DURATION - fadeout_time, FADEOUT_DURATION));

		return;
	}

	

	if (BeingHeld == true)
	{
		float holdDuration = endTime - startTime;
		float Progress = songTime - startTime;
		SetScale(LerpRatio(1.0, 1.3, Progress, holdDuration));
		Green = LerpRatio(0.5, 0.0, Progress, holdDuration);
	}
}

bool GameObject::ShouldRemove()
{
	return AnimationStatus == 3;
}

Judgment GameObject::Run(double delta, double Time, bool Autoplay)
{
	if (fadeout_time || GetPosition().x == 0) // It was hit, so stop.
		return None;

	if (Autoplay)
	{
		// you can be slightly early autoplay, it's fine.
		if (Time >= startTime - 0.008 && fadeout_time == 0 && !BeingHeld) // A pretend kind of thing. ;)
		{
			return Hit(startTime, GetPosition(), true, false, -1);
			           // pretend that all autoplay notes were hit exactly on time.
		}
	}

	// it's not being held? Out of leniency?
	if (Time - startTime > LeniencyHitTime)
	{
		if (endTime == 0)
		{
			fadeout_time = FADEOUT_DURATION; // Fade out! You missed!
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
			fadeout_time = FADEOUT_DURATION;
			BeingHeld = false;
			HoldReleaseSnd->Play();
			return OK;
		}
	}else if (!BeingHeld && Time > startTime+LeniencyHitTime)
	{
		fadeout_time = FADEOUT_DURATION;
		return NG;
	}

	return None;
}

Judgment GameObject::Hit(double Time, Vec2 mpos, bool KeyDown,  bool Autoplay, int32 Key)
{
	Vec2 dist = mpos - GetPosition();
	
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
			Judgment RVal;

			HitSnd->Play();

			if (endTime == 0) // Not a hold?
				fadeout_time = FADEOUT_DURATION;
			else
			{
				heldKey = Key;
				BeingHeld = true;
			}


			// Judgment (values in seconds)
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

void GameObject::Assign(double Duration, uint32 _Measure, double _MeasureFraction)
{
	hold_duration = Duration;
	Measure = _Measure;
	Fraction = _MeasureFraction;
}

/* Assume Sprite was already invalidated */ 
void GameObject::Invalidate()
{
	UvBuffer->Invalidate();
	UpdateTexture();
}

double GameObject::GetFraction() const
{
	return Fraction;
}

void GameObject::SetFraction (double frac)
{
	Fraction = frac;
}

#include "Global.h"
#include "GraphObject2D.h"
#include "GameObject.h"
#include "ImageLoader.h"
#include "FileManager.h"
#include <GL/glfw.h>
#include "Audio.h"

bool GameObject::Initialized = false;

GameObject::GameObject()
{
	setImage(ImageLoader::LoadSkin("hitcircle.png"));
	origin = 1; // use the object's center instead of top-left
	width = height = CircleSize;
	fadeout_time = 0;
	fadein_time = 0.7f;
	hold_duration = 0;
	endTime = 0;
	heldKey = -1;
	BeingHeld = false;

	if (!Initialized) // the catch here is that since it's static, only one vbo is generated ;P
		Init(true);
}

void GameObject::Animate(float delta, float songTime)
{
	if (position.x == 0)
	{
		alpha = 0;
		return;
	}

	if (fadeout_time > 0)
	{
		fadeout_time -= delta*2;

		// alpha out
		alpha = 1 * (fadeout_time);

		if (alpha < 0)
			alpha = 0;

		// scale in
		if (endTime == 0)
			scaleX = scaleY = 2 - fadeout_time;
		else
			scaleX = scaleY = 3 - fadeout_time;

		return;
	}

	if (fadein_time > 0)
	{
		alpha = 1-fadein_time*2;
		fadein_time -= delta;
	}

	if (BeingHeld == true)
	{
		float holdDuration = endTime - startTime;
		float Progress = songTime - startTime;
		scaleX = scaleY = 1 + (0.2*Progress/holdDuration);
		green = 0.5 - 0.5 * Progress/holdDuration;
	}
}

Judgement GameObject::Run(double delta, double Time, bool Autoplay)
{
	if (fadeout_time || position.x == 0) // It was hit, so stop.
		return None;

	if (Autoplay)
	{
		// you can be slightly early autoplay, it's fine.
		if (Time >= startTime - 0.001 && fadeout_time == 0 && !BeingHeld) // A pretend kind of thing. ;)
		{
			return Hit(Time, position, true, false, -1);
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
		if (Time > endTime && fadeout_time == 0)
		{
			fadeout_time = 1;
			audioMgr->play2D((FileManager::GetSkinPrefix() + "holdfinish.ogg").c_str());
			BeingHeld = false;
			return OK;
		}
	}else if (!BeingHeld && Time < endTime-HoldLeniencyHitTime && (Time) > startTime+LeniencyHitTime)
	{
		fadeout_time = 0.7f;
		return NG;
	}

	return None;
}

Judgement GameObject::Hit(float Time, glm::vec2 mpos, bool KeyDown,  bool Autoplay, int Key)
{
	glm::vec2 dist = position - mpos;
	
	if (fadeout_time || position.x == 0 || Autoplay)
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
	if (std::abs(glm::length(dist)) < CircleSize && fadeout_time == 0) 
	{
		float SpareTime = std::abs(Time - startTime);
		if (SpareTime < LeniencyHitTime) // within leniency?
		{
			// Hit!
			Judgement RVal;

			audioMgr->play2D((FileManager::GetSkinPrefix() + "hit.ogg").c_str());
			printf ("songt %f starttime %f difference %f beat %f xpos = %f\n", Time, startTime, Time - startTime, beat, position.x);

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
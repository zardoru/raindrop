
#ifndef GOBJ_H_
#define GOBJ_H_

#include "GraphObject2D.h"

class GameObject : public GraphObject2D
{
	static bool Initialized;
public:
	GameObject();
	bool BeingHeld;
	float fadeout_time, fadein_time, leniency_time; // time to fadeout, and time to get a hit
	float startTime, endTime, beat, hold_duration;
	int heldKey;
	unsigned int Measure, MeasurePos;
	void (*judgementCallback) (Judgement);
	Judgement Hit(float time, glm::vec2 mpos, bool KeyDown, bool Autoplay, int Key);
	Judgement Run(float delta, float Time, bool Autoplay);
	void Animate(float delta, float songTime);
};

#endif
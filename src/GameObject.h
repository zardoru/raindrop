
#ifndef GOBJ_H_
#define GOBJ_H_

#include "GraphObject2D.h"

class GameObject : public GraphObject2D
{
public:
	GameObject();
	void Initialize();

	bool BeingHeld;
	float fadeout_time, fadein_time; // time to fadeout, and time to get a hit
	float waiting_time;

	float startTime, endTime, beat, hold_duration;
	int32 heldKey;

	uint32 Measure, MeasurePos;

	Judgement Hit(float time, glm::vec2 mpos, bool KeyDown, bool Autoplay, int32 Key);
	Judgement Run(double delta, double Time, bool Autoplay);
	void Animate(float delta, float songTime);
	void Invalidate();
	void Render();
};

typedef std::vector<GameObject> GameObjectVector;

#endif
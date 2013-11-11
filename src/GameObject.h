
#ifndef GOBJ_H_
#define GOBJ_H_

#include "GraphObject2D.h"

class GameObject : public GraphObject2D
{
private:
	friend class ScreenEdit;
	friend class Song;
	bool BeingHeld;
	float fadeout_time, fadein_time; // time to fadeout, and time to get a hit
	float waiting_time;

	int32 heldKey;

	float startTime, endTime, beat, hold_duration;
	uint32 Measure, MeasurePos;
	unsigned char AnimationStatus;

public:
	GameObject();
	void Initialize();

	Judgement Hit(float time, glm::vec2 mpos, bool KeyDown, bool Autoplay, int32 Key);
	Judgement Run(double delta, double Time, bool Autoplay);
	void Animate(float delta, float songTime);
	void Assign(float Duration, uint32 Measure, uint32 MeasureFraction);
	bool IsHold();
	void Invalidate();
	void Render();
};

typedef std::vector<GameObject> GameObjectVector;

#endif
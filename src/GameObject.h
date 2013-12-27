
#ifndef GOBJ_H_
#define GOBJ_H_

#include "GraphObject2D.h"

class GameObject : public GraphObject2D
{
private:
	friend class ScreenEdit;
	friend class SongDC;
	bool BeingHeld;
	float fadeout_time, fadein_time; // time to fadeout, and time to get a hit
	float waiting_time;

	int32 heldKey;

	double startTime, endTime, beat, hold_duration;
	uint32 Measure, MeasurePos;
	unsigned char AnimationStatus;

public:
	GameObject();
	void Initialize();

	Judgement Hit(double time, glm::vec2 mpos, bool KeyDown, bool Autoplay, int32 Key);
	Judgement Run(double delta, double Time, bool Autoplay);
	void Animate(float delta, float songTime);
	void Assign(double Duration, uint32 Measure, uint32 MeasureFraction);
	bool IsHold();
	void Invalidate();
	bool ShouldRemove();
};

typedef std::vector<GameObject> GameObjectVector;

#endif